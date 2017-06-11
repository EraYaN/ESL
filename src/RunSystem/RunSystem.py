import paramiko
import sys
import argparse as ap
import getpass
import os
import errno
import stat
import posixpath
import csv
import io
import socket
import time
import getpass
import stat
import time
import math
import re
from operator import itemgetter

try:
    import cPickle as pickle
except:
    import pickle
from terminaltables import AsciiTable

LINE_MARKER = '@'

includes = {
    'shared':{'project':'tracking-shared'},
}

benchmarks = {
    'vanilla':{'project':'tracking', 'executable':'armMeanshiftExec','deps':[],'includes':['shared'],'baseargs':['{basedir}/{testfile}'], 'usesdsp':False, 'output':['/tmp/tracking_result.avi', '/tmp/tracking_result.coords']},
    'dspcore':{'project':'tracking-final-dsp', 'executable':'pool_notify.out', 'deps':[], 'includes':['shared'],'baseargs':[],'sizearg':True, 'usesdsp':True, 'output':[]},
    'final':{'project':'tracking-final', 'executable':'armMeanshiftExec', 'deps':['dspcore'],'includes':['shared'],'baseargs':['{basedir}/{testfile} {basedir}/{depname}'], 'usesdsp':True, 'output':['/tmp/tracking_result.avi', '/tmp/tracking_result.coords', '/tmp/dynrange.csv']},
}

DISASSEMBLY_CMD = 'arm-linux-gnueabi-objdump ~/projects/{project}/{executable} --disassemble -M reg-names-gcc --demangle --endian=little --no-show-raw-insn --reloc'

reference_performance = {
    'testName':'reference',
    'init_time':0.433,
    'kernel_time':10.137,
    'cleanup_time':1.745,
    'total_time':12.318,
    'fps':2.598
}

ignore_strings = [
    '\\obj\\' ,
    '\\bin\\'
]


class Error(Exception):
    """Base class for exceptions in this module."""
    pass

class NotConnectedError(Error):
    """Exception raised for errors with the connection.

    Attributes:
        msg  -- explanation of the error
    """

    def __init__(self, msg):
        self.msg = msg

def is_ignored(name):
    for ignore_string in ignore_strings:
        if ignore_string in name:
            return True
    return False

def upload_files(sftp_client, local_dir, remote_dir):
    if not os.path.exists(local_dir):
        print('Uploading files failed, local path does not exist.')
        return

    local_dir = os.path.realpath(local_dir)
    try:
        if not exists_remote(sftp_client, remote_dir):
            print('Attempting to create {} on the remote.'.format(remote_dir))           
            if mkdir_p(sftp_client,remote_dir):
                print('Created {} on the remote.'.format(remote_dir))
            else:
                print('Could not create {} on the remote.'.format(remote_dir))
    except Exception as e:
        print("Upload_files checking and creating remote directory failed.")
        raise e

    for root, dirs, files in os.walk(local_dir):
        try:
            for dirname in dirs:   
                local_path = os.path.join(root, dirname)
                if is_ignored(local_path):
                    continue;
                remote_path = posixpath.join(remote_dir,os.path.relpath(local_path, local_dir).replace(os.path.sep,posixpath.sep))
                if not exists_remote(sftp_client, remote_path):
                    #print('Attempting to create {} on the
                    #remote.'.format(remote_path))
                    if mkdir_p(sftp_client,remote_path):
                        print('Created {} on the remote.'.format(remote_path))
                    else:
                        print('Could not create {} on the remote.'.format(remote_path))

                #print('Recursing into {}.'.format(local_path))
                #upload_files(sftp_client, local_path, remote_path)
        except Exception as e:
            print("Upload_files creating remote directories failed")
            raise e
        try:
            for filename in files:
                local_path = os.path.join(root, filename)            
                if is_ignored(local_path):
                    continue;
                remote_path = posixpath.join(remote_dir,os.path.relpath(local_path, local_dir).replace(os.path.sep,posixpath.sep))
                if stat.S_ISDIR(os.stat(local_path).st_mode):  
                    print('Found directory?')                
                else:
                    print('Uploading {0} to {1}.'.format(local_path,remote_path))
                    sftp_client.put(local_path,remote_path)
        except Exception as e:
            print("Upload_files section sending files failed")
            raise e

def download_files(sftp_client, remote_dir, local_dir):
    if not exists_remote(sftp_client, remote_dir):
        print('Downloading files failed, remote path does not exist.')
        return

    if not os.path.exists(local_dir):
        os.mkdir(local_dir)

    for filename in sftp_client.listdir(remote_dir):
        if stat.S_ISDIR(sftp_client.stat(remote_dir + filename).st_mode):
            # uses '/' path delimiter for remote server
            download_files(sftp_client, remote_dir + filename + '/', os.path.join(local_dir, filename))
        else:
            if not os.path.isfile(os.path.join(local_dir, filename)):
                sftp_client.get(remote_dir + filename, os.path.join(local_dir, filename))

def download_file(sftp_client, remote_file, local_file):

    if not exists_remote(sftp_client, remote_file):
        print('Downloading file for {0} failed, remote file does not exist.'.format(remote_file))
        return

    local_dir = os.path.dirname(local_file)
    if not os.path.exists(local_dir):
        os.mkdir(local_dir)


    sftp_client.get(remote_file, local_file)

def exists_remote(sftp_client, path):
    try:
        sftp_client.stat(path)
    except IOError as e:
        if e.errno == errno.ENOENT:
            return False
        raise
    else:
        return True

def mkdir_p(sftp, remote_directory):
    """Change to this directory, recursively making new folders if needed.
    Returns True if any folders were created."""
    if remote_directory == '/':
        # absolute path so change directory to root
        sftp.chdir('/')
        return
    if remote_directory == '':
        # top-level relative directory must exist
        return
    try:
        sftp.chdir(remote_directory) # sub-directory exists
    except IOError:
        dirname, basename = posixpath.split(remote_directory)
        mkdir_p(sftp, dirname) # make parent directories
        sftp.mkdir(basename) # sub-directory missing, so created it
        sftp.chdir(basename)
        return True

class RunSystem(object):

    REF_FILE = 'car.avi.refs-coords'
    MUTEX_FILE = '/tmp/mutex/run-system.pid'
    MUTEX_TIMEOUT = 20
    MUTEX_INTERVAL = 1
    LOCAL_BASE_DIR = '..'
    BUILD_BASE_DIR = '~/projects'
    BEAGLE_BASE_DIR = '~/esLAB'

    def __init__(self, benchmark):
        """Return a new RunSystem object."""
        self.build_server = None
        self.transport = None
        self.beagle_board = None
        self.build_channel = None
        self.benchmark = benchmarks[benchmark]
        self.results = []

    def Connect(self,host,port,user,keyfile):
        """Connect to remote server."""
        print("Connecting to BuildServer")
        if keyfile:
            try:
                k = paramiko.RSAKey.from_private_key_file(keyfile)
            except paramiko.PasswordRequiredException:
                k = paramiko.RSAKey.from_private_key_file(keyfile,password=getpass.getpass(prompt='SSH keyfile password: '))
        else:
            k = None

        self.build_server = paramiko.SSHClient()
        self.build_server.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.build_server.connect(hostname=host, port=port, username=user, pkey=k, allow_agent=True)            

        #self.build_server_sftp = self.build_server.open_sftp()
            
        print("Connecting to BeagleBoard")

        # Get the client's transport and open a `direct-tcpip` channel passing
        # the destination hostname:port and the local hostname:port
        self.transport = self.build_server.get_transport()        

        dest_addr = ('192.168.0.202', 22)
        local_addr = ('127.0.0.1', 1234)
        channel = self.transport.open_channel("direct-tcpip", dest_addr, local_addr)       
        
        # Create a NEW client and pass this channel to it as the `sock` (along
        # with
        # whatever credentials you need to auth into your REMOTE box
        self.beagle_board = paramiko.SSHClient()
        self.beagle_board.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.beagle_board.connect('192.168.0.202', port=22, username='root',password='', sock=channel)

        # Get Build home dir.
        print("Getting home directory path on BuildServer")
        output, tmp1, tmp2 = self.BuildExecute('echo ~')
        self.BUILD_BASE_DIR = self.BUILD_BASE_DIR.replace('~',output.strip())

    def Disconnect(self):        
        if self.beagle_board:
            self.beagle_board.close()
            self.transport.close()
        if self.build_server:
            self.build_server.close()

    def BuildExecute(self, cmd):
        try:
            build_channel = self.transport.open_session()
            build_channel.settimeout(10800) 

            # Execute the given command
            build_channel.exec_command(cmd)

            # To capture Data.  Need to read the entire buffer to capture
            # output
            contents = io.StringIO()
            error = io.StringIO()
            time.sleep(0.25)
            while (not build_channel.exit_status_ready()) or build_channel.recv_ready() or build_channel.recv_stderr_ready():
                if build_channel.recv_ready():
                    data = build_channel.recv(1024)
                    #print "Indside stdout"
                    while data:
                        contents.write(data.decode("utf-8"))
                        sys.stdout.write(data.decode("utf-8"))
                        data = build_channel.recv(1024)

                if build_channel.recv_stderr_ready():            
                    error_buff = build_channel.recv_stderr(1024)
                    while error_buff:
                        error.write(error_buff.decode("utf-8"))
                        sys.stderr.write(error_buff.decode("utf-8"))
                        error_buff = build_channel.recv_stderr(1024)            

            exit_status = build_channel.recv_exit_status()

        except socket.timeout:
            raise socket.timeout

        output = contents.getvalue()
        error_value = error.getvalue()

        build_channel.close()

        return output, error_value, exit_status

    def SendSource(self):
        if not self.build_server or not self.beagle_board:
            raise NotConnectedError('Please make sure the class made a successfull connection to the build server and the development board.')
        
        try:             
            sftp = self.build_server.open_sftp()

            for bench in self.benchmark['includes']:
                dep_includes = includes[bench]
                print("Sending sources for include set {project}.".format(**dep_includes))
                local_dir = '{0}/{1}'.format(self.LOCAL_BASE_DIR,dep_includes['project'])
                build_dir = '{0}/{1}'.format(self.BUILD_BASE_DIR,dep_includes['project'])
                upload_files(sftp,local_dir,build_dir)

            for bench in self.benchmark['deps']:
                dep_bench = benchmarks[bench]
                print("Sending sources for depency {project}.".format(**dep_bench))
                local_dir = '{0}/{1}'.format(self.LOCAL_BASE_DIR,dep_bench['project'])
                build_dir = '{0}/{1}'.format(self.BUILD_BASE_DIR,dep_bench['project'])
                upload_files(sftp,local_dir,build_dir)

            print("Sending sources for {project}.".format(**self.benchmark))
            local_dir = '{0}/{1}'.format(self.LOCAL_BASE_DIR,self.benchmark['project'])
            build_dir = '{0}/{1}'.format(self.BUILD_BASE_DIR,self.benchmark['project'])
            upload_files(sftp,local_dir,build_dir)
        except Exception as e:
            import traceback
            sys.stderr.write("Exception: {}\n".format(e))
            traceback.print_exc(file=sys.stderr)
            return False
        finally:
            if sftp:
                sftp.close()
        return True

    def Build(self,makefile):
        if not self.build_server or not self.beagle_board:
            raise NotConnectedError('Please make sure the class made a successfull connection to the build server and the development board.')
        
        for bench in self.benchmark['deps']:
            dep_bench = benchmarks[bench]
            print("Cleaning dependency {project}.".format(**dep_bench))
            out,error,exit_code = self.BuildExecute("make clean -C ~/projects/{project} -f arm.mk".format(**dep_bench))            
            print("Building dependency {project}.".format(**dep_bench))
            out,error,exit_code = self.BuildExecute("make all -C ~/projects/{project} -f arm.mk".format(**dep_bench))            
            if exit_code != 0:
                print('Dependency {0} build failed with exit code {1}'.format(bench,exit_code))
                return False
        bench = self.benchmark
        bench['makefile'] = makefile
        print("Cleaning {project}.".format(**bench))
        out,error,exit_code = self.BuildExecute("make clean -C ~/projects/{project} -f {makefile}".format(**bench))
        print("Building {project}.".format(**bench))
        out,error,exit_code = self.BuildExecute("make all -C ~/projects/{project} -f {makefile}".format(**bench))
        if exit_code != 0:
            print('Main {0} build failed with exit code {1}'.format(self.benchmark['project'],exit_code))
            return False
        
        return True

    def Disassemble(self, filename):
        if not self.build_server:
            raise NotConnectedError('Please make sure the class made a successfull connection to the build server.')
        
        cmd = DISASSEMBLY_CMD.format(**self.benchmark)
        out,error,exit_code = self.BuildExecute(cmd)
        if exit_code != 0:
            print('Disassembly of {0} failed with exit code {1}'.format(self.benchmark['project'],exit_code))
            return False

        #regex = re.compile('\s*([a-z0-9]+):\s*')
        with open(filename, 'w') as f:
            for line in iter(out.splitlines()):
                #matches = regex.match(line)
                f.write(line + '\n')

        return True

    def SendExec(self):
        if not self.build_server or not self.beagle_board:
            raise NotConnectedError('Please make sure the class made a successfull connection to the build server and the development board.')
        
        for bench in self.benchmark['deps']:
            dep_bench = benchmarks[bench]
            print("Sending compiled dependency {project} to BeagleBoard.".format(**dep_bench))
            out,error,exit_code = self.BuildExecute("make send -C ~/projects/{project} -f arm.mk".format(**dep_bench))
            if exit_code != 0:
                print('Sending {0} failed with exit code {1}'.format(dep_bench['project'],exit_code))
                return False

        print("Sending compiled {project} to BeagleBoard.".format(**self.benchmark))
        out,error,exit_code = self.BuildExecute("make send -C ~/projects/{project} -f arm.mk".format(**self.benchmark))
        if exit_code != 0:
            print('Sending {0} failed with exit code {1}'.format(self.benchmark['project'],exit_code))
            return False 

        return True

    def TryMutexLock(self, sftp):
        return not exists_remote(sftp,self.MUTEX_FILE)        

    def WaitForMutexLock(self, sftp):
        start_time = time.time()
        while True:
            pid = -1
            user = 'none'
            if exists_remote(sftp,self.MUTEX_FILE):
                file = sftp.file(self.MUTEX_FILE, "r", -1)
                pid = int(file.readline().strip())
                user = file.readline().strip()                
                file.close()
                if not exists_remote(sftp,"/proc/{}".format(pid)): 
                    self.ResetMutexLock(sftp)
                    print('Mutex lock acquired from {} by {}, the process already quit.'.format(pid,user))
                    return True
                if start_time + self.MUTEX_TIMEOUT < time.time():
                    print('Mutex Lock timed out for process {} by {}.'.format(pid,user))
                    return False
            else:
                print('Mutex lock acquired.'.format(pid,user))
                return True
            print('Waiting for process {} by {} to exit.'.format(pid,user))
            time.sleep(self.MUTEX_INTERVAL)

    def SetMutexLock(self, sftp):
        try:
            pid = os.getpid()
            file = sftp.file(self.MUTEX_FILE, "w", -1)
            file.chmod(stat.S_IRUSR|stat.S_IRGRP|stat.S_IROTH|stat.S_IWUSR|stat.S_IWGRP|stat.S_IWOTH|stat.S_IXUSR|stat.S_IXGRP|stat.S_IXOTH)
            file.write("{}\n".format(pid))
            file.write("{}\n".format(getpass.getuser()))
            file.flush()
            
        except Exception:
            print('Could not create mutex lock file.')
            return False
        finally:
            if file:
                file.close()
        print('Created mutex lock file successfully.')
        return True

    def ResetMutexLock(self, sftp):
        if exists_remote(sftp,self.MUTEX_FILE):
            code = sftp.remove(self.MUTEX_FILE)
            if code == paramiko.SFTP_OK or code == None:                
                print('Reset mutex lock file successfully.')
                return True
            else:
                print('Could not remove mutex lock file, code: {0}.'.format(code))
                return False
        else:
            print('Mutex lock file does not exist.')
            return True

    def Run(self, file):
        if not self.build_server or not self.beagle_board:
            raise NotConnectedError('Please make sure the class made a successfull connection to the build server and the development board.')
        
        try: 
            sftp_build = self.build_server.open_sftp()
            print("Waiting for Mutex Lock....")
            if not self.WaitForMutexLock(sftp_build):
                return False
            print("Setting for Mutex Lock....")
            if not self.SetMutexLock(sftp_build):
                return False
            if self.benchmark['usesdsp']:
                print("Powercycling DSP on BeagleBoard.")
                ssh_stdin, ssh_stdout, ssh_stderr = self.beagle_board.exec_command("~/powercycle.sh")        
                self.PrintCommandOutput(ssh_stdout,ssh_stderr)

            print("Executing project {project} on BeagleBoard.".format(**self.benchmark))
            formatarguments = {}
            formatarguments['basedir'] = self.BEAGLE_BASE_DIR
            formatarguments['executable'] = self.benchmark['executable']
            print("Using file {0}".format(file))
            formatarguments['testfile'] = file
            formatarguments['depname'] = ''
            if len(self.benchmark['deps']) >= 1:
                formatarguments['depname'] = benchmarks[self.benchmark['deps'][0]]['executable']        

            formatarguments['arguments'] = [basearg.format(**formatarguments) for basearg in self.benchmark['baseargs']]   
        
            formatarguments['arguments_str'] = ' '.join(formatarguments['arguments'])
            print("{basedir}/{executable} {arguments_str}".format(**formatarguments))
            ssh_stdin, ssh_stdout, ssh_stderr = self.beagle_board.exec_command("cd {basedir}; {basedir}/{executable} {arguments_str}".format(**formatarguments))        
            #self.PrintCommandOutput(ssh_stdout,ssh_stderr)
            csv_data = ''
            for line in ssh_stdout: #read and store result in log file
                if line[0:1] == LINE_MARKER:
                    csv_data+=line[1:]
                sys.stdout.write('#stdout {}'.format(line))

            for line in ssh_stderr: #read and store result in log file
                if line[0:1] == LINE_MARKER:
                    csv_data+=line[1:]
                sys.stderr.write('#stderr {}'.format(line))
            reader = csv.reader(csv_data.splitlines())
            result = {}
            for testName,init_time,kernel_time,cleanup_time,total_time,fps in reader:
                testName = '{}-{}'.format(testName, file)
                result = {'testName':testName,'init_time':float(init_time),'kernel_time':float(kernel_time),'cleanup_time':float(cleanup_time),'total_time':float(total_time),'fps':float(fps)}            

            result['error'] = 0
            if self.benchmark['output']:
                try: 
                    sftp = self.beagle_board.open_sftp()

                    for filename in self.benchmark['output']:
                        formattedfilename = filename.format(**formatarguments)          
                    
                        local_path = formattedfilename.replace('~', '.').replace('/tmp', '.').replace(posixpath.sep, os.path.sep)
                        print("Downloading file {}....".format(formattedfilename))
                        download_file(sftp, formattedfilename, local_path)
                        if os.path.splitext(formattedfilename)[1] == '.coords':
                            result['error'] += self.Verify(local_path)
                finally:
                    if sftp:
                        sftp.close()

            self.results.append(result)
            if not self.ResetMutexLock(sftp_build):
                return False
            return True
        except Exception as e:
            import traceback
            sys.stderr.write("Exception: {}\n".format(e))
            traceback.print_exc(file=sys.stderr)
            return False
        finally:
            if sftp_build:
                sftp_build.close()
        return True

    def Verify(self, file):
        if not os.path.exists(file):
            print('Verification file does not exist.')
            return -1
        if not os.path.exists(self.REF_FILE):
            print('Reference file does not exist.')
            return -1
        ref = []
        res = []
        with open(self.REF_FILE, 'r') as f:
            reader = csv.reader(f)
            next(reader, None)
            for f,x,y in reader:
                ref.append({'f':int(f),'x':int(x),'y':int(y)})

        with open(file, 'r') as f:
            reader = csv.reader(f)
            next(reader, None)
            for f,x,y in reader:
                res.append({'f':int(f),'x':int(x),'y':int(y)})

        if len(res) == 0:
            print('There are no data points in the verification file... Run must have had an error.')
            return -1

        if len(res) > len(ref):
            print('There are more data points in the verification file then in the reference file.')

        ref = sorted(ref, key=itemgetter('f'))
        res = sorted(res, key=itemgetter('f'))
        error = 0
        for i in range(0,len(ref)):
            res_p = res[i]
            ref_p = ref[i]
            if res_p['f'] != ref_p['f']:
                print('Item {0} in the two files does not have the same framenumber.'.format(i))

            error_x = ref_p['x'] - res_p['x']
            error_y = ref_p['y'] - res_p['y']
            error += math.sqrt(error_x**2+error_y**2)

        return error        

    def RunN(self, file, n=5):
        for i in range(0,n):
            print("Doing run {0} out of {1}.".format(i + 1,n))
            if not self.Run(file):
                return False
        return True

    def PrintCommandOutput(self, stdout, stderr):
        out = stdout.readlines()
        err = stderr.readlines()
        for l in out:
            sys.stdout.write('#stdout {}'.format(l))
        for l in err:
            sys.stderr.write('#stderr {}'.format(l))
        sys.stdout.flush()
        sys.stderr.flush()

    def SaveResults(self, filename):
        print('Saving Results...')
        with open(filename, 'wb') as handle:
            pickle.dump(self.results, handle, protocol=pickle.HIGHEST_PROTOCOL)

    def PrintResults(self):
        table_heading = ['Test',
            'Avg',
            'Passed',
            'tI',
            'tK',
            'tC',
            'tT',
            'fps',            
            'Error',
            'sI',
            'sK',
            'sC',
            'sT']
        table_justify = {
            0:'left',
            1:'left',
            2:'left',
            3:'right',
            4:'right',
            5:'right',
            6:'right', 
            7:'right',
            8:'right',
            9:'right',
            10:'right',  
            11:'right',                        
            12:'right'
        }
        display_results = []
        display_results.append(table_heading)
        sorted_results = {}
        for result in self.results:            
            display_results.append([result['testName'],
            'No',
            'Yes' if result['error'] == 0 else 'No',
            "{0:.3f} s".format(result['init_time']),
            "{0:.3f} s".format(result['kernel_time']),
            "{0:.3f} s".format(result['cleanup_time']),
            "{0:.3f} s".format(result['total_time']),
            "{0:.3f}".format(result['fps']),
            "{0:.3f}".format(result['error']),
            "{0:.1f}x".format(reference_performance['init_time'] / result['init_time']),
            "{0:.1f}x".format(reference_performance['kernel_time'] / result['kernel_time']),
            "{0:.1f}x".format(reference_performance['cleanup_time'] / result['cleanup_time']),
            "{0:.1f}x".format(reference_performance['total_time'] / result['total_time'])])

            if result['testName'] not in sorted_results:
                sorted_results[result['testName']] = []

            sorted_results[result['testName']].append(result)
        
        for testName in sorted_results:    
            count = len(sorted_results[result['testName']])            
            display_results.append([result['testName'],
            'Yes',
            'Yes' if sum(item['error'] for item in sorted_results[result['testName']]) / count == 0 else 'No',
            "{0:.3f} s".format(sum(item['init_time'] for item in sorted_results[result['testName']]) / count),
            "{0:.3f} s".format(sum(item['kernel_time'] for item in sorted_results[result['testName']]) / count),
            "{0:.3f} s".format(sum(item['cleanup_time'] for item in sorted_results[result['testName']]) / count),
            "{0:.3f} s".format(sum(item['total_time'] for item in sorted_results[result['testName']]) / count),
            "{0:.3f}".format(sum(item['fps'] for item in sorted_results[result['testName']]) / count),
            "{0:.3f}".format(sum(item['error'] for item in sorted_results[result['testName']]) / count),
            "{0:.1f}x".format(sum(reference_performance['init_time'] / item['init_time'] for item in sorted_results[result['testName']]) / count),
            "{0:.1f}x".format(sum(reference_performance['kernel_time'] / item['kernel_time'] for item in sorted_results[result['testName']]) / count),
            "{0:.1f}x".format(sum(reference_performance['cleanup_time'] / item['cleanup_time'] for item in sorted_results[result['testName']]) / count),
            "{0:.1f}x".format(sum(reference_performance['total_time'] / item['total_time'] for item in sorted_results[result['testName']]) / count)])

        results_table = AsciiTable(display_results)
        results_table.justify_columns = table_justify
        print(results_table.table)

if __name__ == '__main__':
    parser = ap.ArgumentParser(prog='ESLab RunSystem',description='ESLab RunSystem')
    parser.add_argument('--key-file', action="store", help='SSH Key file',default=None)
    parser.add_argument('--user', action="store", help='SSH Username (default erwin)',default="erwin")
    parser.add_argument('--host', action="store", help='SSH Host (default erayan.duckdns.org)',default="erayan.duckdns.org")
    parser.add_argument('--port', action="store", help='SSH Port (default 34522)',default=34522)
    parser.add_argument('--sendsource', action="store_true", help='Should we upload the source to the BuildServer')
    parser.add_argument('--build', action="store_true", help='Should we build the executable for the BeagleBoard')
    parser.add_argument('--disassemble', action="store_true", help='Should we get the disassembly, requires --build.')
    parser.add_argument('--run', action="store_true", help='Should we run the executable on the BeagleBoard')
    parser.add_argument('--files', action="store", help='The test files to use in this run.', default='car.avi')
    parser.add_argument('--number-of-runs', action="store", type=int, help='The number of runs to do.', default=1)
    parser.add_argument('--benchmark', action="store", help='Benchmark to run',choices=list(benchmarks.keys()),default='vanilla')
    parser.add_argument('--variant', action="store", help='Variant name, used to save the results',default='default')
    parser.add_argument('--makefile', action="store", help='Makefile name, used to build. Dependencies are always built with arm.mk',default='arm.mk')
    print("Starting...")
    try:
        opts = parser.parse_args(sys.argv[1:])
        try:
            rs = RunSystem(benchmark=opts.benchmark)
            rs.Connect(opts.host,opts.port,opts.user,opts.key_file)
            if opts.sendsource:
                if not rs.SendSource():
                    print('SendSource Failed.')
                    exit(1)
            if opts.build:
                if not rs.Build(opts.makefile):
                    print('Build Failed.')
                    exit(2)
                if opts.disassemble:
                    if not rs.Disassemble("{0}-{2}-{1}.disassembly.txt".format(opts.benchmark,opts.files.replace(',','_'),opts.variant)):
                        print('Disassemble Failed.')
                        exit(5)

                if not rs.SendExec():
                    print('SendExec Failed.')
                    exit(3)
            if opts.run:
                files = opts.files.split(',')
                for file in files:
                    if not rs.RunN(file,opts.number_of_runs):
                        print('RunN Failed.')
                        exit(4)
                rs.SaveResults("{0}-{2}-{1}.pickle".format(opts.benchmark,opts.files.replace(',','_'),opts.variant))

                rs.PrintResults()

        finally:
            if rs:
                rs.Disconnect()

        print("Done.")
    except SystemExit:
        print('Bad Arguments')