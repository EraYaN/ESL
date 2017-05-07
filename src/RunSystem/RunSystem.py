import paramiko
import sys
import argparse as ap
import getpass
import os
import errno
import stat
import posixpath
import csv
try:
    import cPickle as pickle
except:
    import pickle
from terminaltables import AsciiTable

LOCAL_BASE_DIR = '..'
BUILD_BASE_DIR = '~/projects'
BEAGLE_BASE_DIR = '~/esLAB'
LINE_MARKER = '@'

includes = {
    'shared':{'project':'matrix-include'},
}

benchmarks = {
    'vanilla':{'project':'matrix', 'executable':'matrix','deps':[],'includes':['shared'],'baseargs':[],'sizearg':False, 'usesdsp':False},
    'dynamic':{'project':'matrix-dynamic', 'executable':'matrix-mult-dynamic','deps':[],'includes':['shared'],'baseargs':[],'sizearg':True, 'usesdsp':False},
    'neon':{'project':'matrix-neon', 'executable':'matrix-mult-neon', 'deps':[],'includes':['shared'],'baseargs':[],'sizearg':True, 'usesdsp':False},
    'dsp':{'project':'matrix-dsp', 'executable':'matrix.out', 'deps':[], 'includes':['shared'],'baseargs':[],'sizearg':True, 'usesdsp':True},
    'gpp':{'project':'matrix-gpp', 'executable':'matrixgpp', 'deps':['dsp'],'includes':['shared'],'baseargs':['{basedir}/{depname}'],'sizearg':True, 'usesdsp':True},
    }

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

def upload_files(sftp_client, local_dir, remote_dir):
    if not os.path.exists(local_dir):
        print('Uploading files failed, local path does not exist.')
        return

    local_dir = os.path.realpath(local_dir)
    remote_home_dir = sftp_client.normalize('.')
    remote_dir = remote_dir.replace('~',remote_home_dir)

    if not exists_remote(sftp_client, remote_dir):
        #print('Attempting to create {} on the remote.'.format(remote_dir))
        if mkdir_p(sftp_client,remote_dir):
            print('Created {} on the remote.'.format(remote_dir))
        else:
            print('Could not create {} on the remote.'.format(remote_dir))    

    for root, dirs, files in os.walk(local_dir):
        for dirname in dirs:
            local_path = os.path.join(root, dirname)
            remote_path = posixpath.join(remote_dir,os.path.relpath(local_path, local_dir).replace(os.path.sep,posixpath.sep))
            if not exists_remote(sftp_client, remote_path):
                #print('Attempting to create {} on the remote.'.format(remote_path))
                if mkdir_p(sftp_client,remote_path):
                    print('Created {} on the remote.'.format(remote_path))
                else:
                    print('Could not create {} on the remote.'.format(remote_path))

            #print('Recursing into {}.'.format(local_path))
            #upload_files(sftp_client, local_path, remote_path)

        for filename in files:
            local_path = os.path.join(root, filename)
            remote_path = posixpath.join(remote_dir,os.path.relpath(local_path, local_dir).replace(os.path.sep,posixpath.sep))
            if stat.S_ISDIR(os.stat(local_path).st_mode):  
                print('Found directory?')
                
            else:
                print('Uploading {0} to {1}.'.format(local_path,remote_path))
                sftp_client.put(local_path,remote_path)

def download_files(sftp_client, remote_dir, local_dir):
    if not exists_remote(sftp_client, remote_dir):
        print('Uploading files failed, remote path does not exist.')
        return

    if not os.path.exists(local_dir):
        os.mkdir(local_dir)

    for filename in sftp_client.listdir(remote_dir):
        if stat.S_ISDIR(sftp_client.stat(remote_dir + filename).st_mode):
            # uses '/' path delimiter for remote server
            download_file(sftp_client, remote_dir + filename + '/', os.path.join(local_dir, filename))
        else:
            if not os.path.isfile(os.path.join(local_dir, filename)):
                sftp_client.get(remote_dir + filename, os.path.join(local_dir, filename))

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

    def __init__(self, benchmark):
        """Return a new RunSystem object."""
        self.build_server = None
        self.transport = None
        self.beagle_board = None
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
        
        # Create a NEW client and pass this channel to it as the `sock` (along with
        # whatever credentials you need to auth into your REMOTE box
        self.beagle_board = paramiko.SSHClient()
        self.beagle_board.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.beagle_board.connect('192.168.0.202', port=22, username='root',password='', sock=channel)
                    


    def Disconnect(self):        
        if self.beagle_board:
            self.beagle_board.close()
            self.transport.close()
        if self.build_server:
            self.build_server.close()

    def SendSource(self):
        if not self.build_server or not self.beagle_board:
            raise NotConnectedError('Please make sure the class made a successfull connection to the build server and the development board.')
        
        try: 
            sftp = self.build_server.open_sftp()

            for bench in self.benchmark['includes']:
                dep_includes = includes[bench]
                print("Sending sources for include set {project}.".format(**dep_includes))
                local_dir = '{0}/{1}'.format(LOCAL_BASE_DIR,dep_includes['project'])
                build_dir = '{0}/{1}'.format(BUILD_BASE_DIR,dep_includes['project'])
                upload_files(sftp,local_dir,build_dir)

            for bench in self.benchmark['deps']:
                dep_bench = benchmarks[bench]
                print("Sending sources for depency {project}.".format(**dep_bench))
                local_dir = '{0}/{1}'.format(LOCAL_BASE_DIR,dep_bench['project'])
                build_dir = '{0}/{1}'.format(BUILD_BASE_DIR,dep_bench['project'])
                upload_files(sftp,local_dir,build_dir)

            print("Sending sources for {project}.".format(**self.benchmark))
            local_dir = '{0}/{1}'.format(LOCAL_BASE_DIR,self.benchmark['project'])
            build_dir = '{0}/{1}'.format(BUILD_BASE_DIR,self.benchmark['project'])
            upload_files(sftp,local_dir,build_dir)
        finally:
            if sftp:
                sftp.close()

    def Build(self):
        if not self.build_server or not self.beagle_board:
            raise NotConnectedError('Please make sure the class made a successfull connection to the build server and the development board.')
        
        for bench in self.benchmark['deps']:
            dep_bench = benchmarks[bench]
            print("Cleaning dependency {project}.".format(**dep_bench))
            ssh_stdin, ssh_stdout, ssh_stderr = self.build_server.exec_command("make clean -C ~/projects/{project}".format(**dep_bench))
            self.PrintCommandOutput(ssh_stdout,ssh_stderr)
            print("Building dependency {project}.".format(**dep_bench))
            ssh_stdin, ssh_stdout, ssh_stderr = self.build_server.exec_command("make all -C ~/projects/{project}".format(**dep_bench))
            self.PrintCommandOutput(ssh_stdout,ssh_stderr)

        print("Cleaning {project}.".format(**self.benchmark))
        ssh_stdin, ssh_stdout, ssh_stderr = self.build_server.exec_command("make clean -C ~/projects/{project}".format(**self.benchmark))
        self.PrintCommandOutput(ssh_stdout,ssh_stderr)
        print("Building {project}.".format(**self.benchmark))
        ssh_stdin, ssh_stdout, ssh_stderr = self.build_server.exec_command("make all -C ~/projects/{project}".format(**self.benchmark))
        self.PrintCommandOutput(ssh_stdout,ssh_stderr)       

    def SendExec(self):
        if not self.build_server or not self.beagle_board:
            raise NotConnectedError('Please make sure the class made a successfull connection to the build server and the development board.')
        
        for bench in self.benchmark['deps']:
            dep_bench = benchmarks[bench]
            print("Sending compiled dependency {project} to BeagleBoard.".format(**dep_bench))
            ssh_stdin, ssh_stdout, ssh_stderr = self.build_server.exec_command("make send -C ~/projects/{project}".format(**dep_bench))
            self.PrintCommandOutput(ssh_stdout,ssh_stderr)   

        print("Sending compiled {project} to BeagleBoard.".format(**self.benchmark))
        ssh_stdin, ssh_stdout, ssh_stderr = self.build_server.exec_command("make send -C ~/projects/{project}".format(**self.benchmark))
        self.PrintCommandOutput(ssh_stdout,ssh_stderr)   

    def Run(self, size=64):
        if not self.build_server or not self.beagle_board:
            raise NotConnectedError('Please make sure the class made a successfull connection to the build server and the development board.')
        
        if self.benchmark['usesdsp']:
            print("Powercycling DSP on BeagleBoard.")
            ssh_stdin, ssh_stdout, ssh_stderr = self.beagle_board.exec_command("~/powercycle.sh")        
            self.PrintCommandOutput(ssh_stdout,ssh_stderr)

        print("Executing project {project} on BeagleBoard.".format(**self.benchmark))
        formatarguments = {}
        formatarguments['basedir'] = BEAGLE_BASE_DIR
        formatarguments['executable'] = self.benchmark['executable']
        formatarguments['depname'] = ''
        if len(self.benchmark['deps']) >= 1:
            formatarguments['depname'] = benchmarks[self.benchmark['deps'][0]]['executable']
        

        formatarguments['arguments'] = [basearg.format(**formatarguments) for basearg in self.benchmark['baseargs']]
        if self.benchmark['sizearg']:
            print("Using size {0}".format(size))
            formatarguments['arguments'].append('{0:d}'.format(size))

        formatarguments['arguments_str'] = ' '.join(formatarguments['arguments'])

        ssh_stdin, ssh_stdout, ssh_stderr = self.beagle_board.exec_command("{basedir}/{executable} {arguments_str}".format(**formatarguments))        
        self.PrintCommandOutput(ssh_stdout,ssh_stderr)
        csv_data = ''
        for line in ssh_stdout: #read and store result in log file
            if line[0:1] == LINE_MARKER:
                csv_data+=line[1:]
        for line in ssh_stderr: #read and store result in log file
            if line[0:1] == LINE_MARKER:
                csv_data+=line[1:]

        reader = csv.reader(csv_data.splitlines())
        for testName,init_time,kernel_time,cleanup_time,total_time in reader:
            if self.benchmark['sizearg']:
                testName = '{}({})'.format(testName,size)
            self.results.append({'testName':testName,'init_time':init_time,'kernel_time':kernel_time,'cleanup_time':cleanup_time,'total_time':total_time})

    def RunN(self, size=64, n=5):
        for i in range(0,n):
            print("Doing run {0} out of {1}.".format(i+1,n))
            self.Run(size)

    def SaveResults(self, filename):
        print('Saving Results...')
        with open(filename, 'wb') as handle:
            pickle.dump(self.results, handle, protocol=pickle.HIGHEST_PROTOCOL)

    def PrintCommandOutput(self, stdout, stderr):
        out = stdout.readlines()
        err = stderr.readlines()
        for l in out:
            sys.stdout.write('#stdout {}'.format(l))
        for l in err:
            sys.stderr.write('#stderr {}'.format(l))
        sys.stdout.flush()
        sys.stderr.flush()

    def PrintResults(self):
        table_heading = [
            'test',
            'isAverage',
            'iT',
            'tK',
            'tC',
            'tT'
            ]
        table_justify = {
            0:'left',
            1:'left',
            2:'right',
            3:'right',
            4:'right',
            5:'right'
        }
        display_results = []
        display_results.append(table_heading)
        sorted_results = []
        for result in self.results:            
            display_results.append([
            result['testName'],
            'No',
            "{0:.3f} s".format(result['init_time']),
            "{0:.3f} s".format(result['kernel_time']),
            "{0:.3f} s".format(result['cleanup_time']),
            "{0:.3f} s".format(result['total_time'])
            ])

            if result['testName'] not in sorted_results:
                sorted_results[result['testName']] = []

            sorted_results[result['testName']].append(result)
        
        for testName in sorted_results:    
            count = len(sorted_results['testName'])            
            display_results.append([
            result['testName'],
            'Yes',
            "{0:.3f} s".format(sum(item['init_time'] for item in sorted_results['testName'])/count),
            "{0:.3f} s".format(sum(item['kernel_time'] for item in sorted_results['testName'])/count),
            "{0:.3f} s".format(sum(item['cleanup_time'] for item in sorted_results['testName'])/count),
            "{0:.3f} s".format(sum(item['total_time'] for item in sorted_results['testName'])/count)
            ])

        results_table = AsciiTable(display_results)
        results_table.justify_columns = table_justify
        print(results_table.table)

if __name__ == '__main__':
    parser = ap.ArgumentParser(prog='ESLab RunSystem',description='ESLab RunSystem')
    parser.add_argument('--key-file', action="store", help='SSH Key file',default=None)
    parser.add_argument('--user', action="store", help='SSH Username',default="erwin")
    parser.add_argument('--host', action="store", help='SSH Host',default="erayan.duckdns.org")
    parser.add_argument('--port', action="store", help='SSH Port',default=34522)
    parser.add_argument('--sendsource', action="store_true", help='Should we upload the source to the BuildServer')
    parser.add_argument('--build', action="store_true", help='Should we build the executable for the BeagleBoard')
    parser.add_argument('--run', action="store_true", help='Should we run the executable on the BeagleBoard')
    parser.add_argument('--sweep', action="store", help='The sweep matrix sizes in comma seperated list, if the benchmark supports the size argument.', default='64')
    parser.add_argument('--number-of-runs', action="store", type=int, help='The number of runs to do.', default=1)
    parser.add_argument('--benchmark', action="store", help='Benchmark to run',choices=list(benchmarks.keys()),default='vanilla')
    parser.add_argument('--variant', action="store", help='Variant name, used to save the results',default='default')
    print("Starting...")
    try:
        opts = parser.parse_args(sys.argv[1:])
        try:
            rs = RunSystem(benchmark=opts.benchmark)
            rs.Connect(opts.host,opts.port,opts.user,opts.key_file)
            if opts.sendsource:
                rs.SendSource()
            if opts.build:
                rs.Build()
                rs.SendExec()
            if opts.run:
                sweep_sizes = opts.sweep.split(',')
                for sweep_size in sweep_sizes:
                    rs.RunN(int(sweep_size),opts.number_of_runs)
                rs.SaveResults("{0}-{2}-{1}.pickle".format(opts.benchmark,opts.sweep.replace(',','_'),opts.variant))
                
                rs.PrintResults()
        finally:
            if rs:
                rs.Disconnect()

        print("Done.")
    except SystemExit:
        print('Bad Arguments')