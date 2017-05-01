print("Starting...")
import paramiko
import sys
import argparse as ap

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

class RunSystem(object):

    def __init__(self):
        """Return a new RunSystem object."""
        self.build_server = None
        self.transport = None
        self.beagle_board = None

    def Connect(self,host,port,user,keyfile):
        """Connect to remote server."""
        print("Connecting to BuildServer")
        if keyfile:
            k = paramiko.RSAKey.from_private_key_file(keyfile)
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

    def Run(self, projectname='matrix', arguments='5'):
        if not self.build_server or not self.beagle_board:
            raise NotConnectedError('Please make sure the class made a successfull connection to the build server and the development board.')
        
        print("Sending compiled {}-gpp to BeagleBoard.".format(projectname))
        ssh_stdin, ssh_stdout, ssh_stderr = self.build_server.exec_command("make send -C ~/projects/{}-gpp".format(projectname))
        self.PrintCommandOutput(ssh_stdout,ssh_stderr)
        print("Sending compiled {}-dsp to BeagleBoard.".format(projectname))
        ssh_stdin, ssh_stdout, ssh_stderr = self.build_server.exec_command("make send -C ~/projects/{}-dsp".format(projectname))
        self.PrintCommandOutput(ssh_stdout,ssh_stderr)
        print("Powercycling DSP on BeagleBoard.")
        ssh_stdin, ssh_stdout, ssh_stderr = self.beagle_board.exec_command("~/powercycle.sh")        
        self.PrintCommandOutput(ssh_stdout,ssh_stderr)
        print("Executing project {} on BeagleBoard.".format(projectname))
        ssh_stdin, ssh_stdout, ssh_stderr = self.beagle_board.exec_command("~/esLAB/{0}gpp ~/esLAB/{0}.out {1}".format(projectname,arguments))        
        self.PrintCommandOutput(ssh_stdout,ssh_stderr)

    def PrintCommandOutput(self, stdout, stderr):
        out = stdout.readlines()
        err = stderr.readlines()
        for l in out:
            sys.stdout.write(l)
        for l in err:
            sys.stderr.write(l)
        sys.stdout.flush()
        sys.stderr.flush()

if __name__ == '__main__':
    parser = ap.ArgumentParser(prog='ESLab RunSystem',description='ESLab RunSystem')
    parser.add_argument('--key-file', action="store", help='SSH Key file',default=None)
    parser.add_argument('--user', action="store", help='SSH Username',default="erwin")
    parser.add_argument('--host', action="store", help='SSH Host',default="erayan.duckdns.org")
    parser.add_argument('--port', action="store", help='SSH Port',default=34522)
    try:
        opts = parser.parse_args(sys.argv[1:])        
        try:
            rs = RunSystem()
            rs.Connect(opts.host,opts.port,opts.user,opts.key_file)
            rs.Run()
        finally:
            if rs:
                rs.Disconnect()

        print("Done.")
    except SystemExit:
        print('Bad Arguments')