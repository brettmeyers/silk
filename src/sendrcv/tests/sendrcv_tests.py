#!/usr/bin/env python
#######################################################################
# Copyright (C) 2008-2015 by Carnegie Mellon University.
#
# @OPENSOURCE_HEADER_START@
#
# Use of the SILK system and related source code is subject to the terms
# of the following licenses:
#
# GNU Public License (GPL) Rights pursuant to Version 2, June 1991
# Government Purpose License Rights (GPLR) pursuant to DFARS 252.227.7013
#
# NO WARRANTY
#
# ANY INFORMATION, MATERIALS, SERVICES, INTELLECTUAL PROPERTY OR OTHER
# PROPERTY OR RIGHTS GRANTED OR PROVIDED BY CARNEGIE MELLON UNIVERSITY
# PURSUANT TO THIS LICENSE (HEREINAFTER THE "DELIVERABLES") ARE ON AN
# "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY
# KIND, EITHER EXPRESS OR IMPLIED AS TO ANY MATTER INCLUDING, BUT NOT
# LIMITED TO, WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE,
# MERCHANTABILITY, INFORMATIONAL CONTENT, NONINFRINGEMENT, OR ERROR-FREE
# OPERATION. CARNEGIE MELLON UNIVERSITY SHALL NOT BE LIABLE FOR INDIRECT,
# SPECIAL OR CONSEQUENTIAL DAMAGES, SUCH AS LOSS OF PROFITS OR INABILITY
# TO USE SAID INTELLECTUAL PROPERTY, UNDER THIS LICENSE, REGARDLESS OF
# WHETHER SUCH PARTY WAS AWARE OF THE POSSIBILITY OF SUCH DAMAGES.
# LICENSEE AGREES THAT IT WILL NOT MAKE ANY WARRANTY ON BEHALF OF
# CARNEGIE MELLON UNIVERSITY, EXPRESS OR IMPLIED, TO ANY PERSON
# CONCERNING THE APPLICATION OF OR THE RESULTS TO BE OBTAINED WITH THE
# DELIVERABLES UNDER THIS LICENSE.
#
# Licensee hereby agrees to defend, indemnify, and hold harmless Carnegie
# Mellon University, its trustees, officers, employees, and agents from
# all claims or demands made against them (and any related losses,
# expenses, or attorney's fees) arising out of, or relating to Licensee's
# and/or its sub licensees' negligent use or willful misuse of or
# negligent conduct or willful misconduct regarding the Software,
# facilities, or other rights or assistance granted by Carnegie Mellon
# University under this License, including, but not limited to, any
# claims of product liability, personal injury, death, damage to
# property, or violation of any laws or regulations.
#
# Carnegie Mellon University Software Engineering Institute authored
# documents are sponsored by the U.S. Department of Defense under
# Contract FA8721-05-C-0003. Carnegie Mellon University retains
# copyrights in all material produced under this contract. The U.S.
# Government retains a non-exclusive, royalty-free license to publish or
# reproduce these documents, or allow others to do so, for U.S.
# Government purposes only pursuant to the copyright license under the
# contract clause at 252.227.7013.
#
# @OPENSOURCE_HEADER_END@
#
#######################################################################

#######################################################################
# $SiLK: sendrcv_tests.py b7b8edebba12 2015-01-05 18:05:21Z mthomas $
#######################################################################

import sys
import re
import os
import os.path
import tempfile
import random
import stat
import fcntl
import re
import shutil
import itertools
import optparse
import time
import signal
import select
import subprocess
import socket
import json
import traceback
import struct
import datetime

conv = json

srcdir = os.environ.get("srcdir")
if srcdir:
    sys.path.insert(0, os.path.join(srcdir, "tests"))
from gencerts import reset_all_certs_and_keys, generate_signed_cert, generate_ca_cert
from config_vars import config_vars
from daemon_test import get_ephemeral_port, Dirobject

try:
    import hashlib
    md5_new = hashlib.md5
    sha1_new = hashlib.sha1
except ImportError:
    import md5
    md5_new = md5.new
    import sha
    sha1_new = sha.new


global_int     = 0

KILL_DELAY     = 20
CHUNKSIZE      = 2048
OVERWRITE      = False
LOG_LEVEL      = "info"
LOG_OUTPUT     = []

# do not remove the directory after the test
NO_REMOVE      = False
FILE_LIST_FILE = None

# tests to run (in the order in which to run them) if no tests are
# specified on the command line
ALL_TESTS = ['testConnectOnlyIPv4Addr', 'testConnectOnlyHostname',
             'testConnectOnlyTLS', 'testConnectOnlyIPv6Addr',
             'testSendRcvStopReceiverServer', 'testSendRcvStopSenderServer',
             'testSendRcvStopReceiverClient', 'testSendRcvStopSenderClient',
             'testSendRcvKillReceiverServer', 'testSendRcvKillSenderServer',
             'testSendRcvKillReceiverClient', 'testSendRcvKillSenderClient',
             'testSendRcvStopReceiverServerTLS',
             'testSendRcvStopSenderServerTLS',
             'testSendRcvStopReceiverClientTLS',
             'testSendRcvStopSenderClientTLS',
             'testSendRcvKillReceiverServerTLS',
             'testSendRcvKillSenderServerTLS',
             'testSendRcvKillReceiverClientTLS',
             'testSendRcvKillSenderClientTLS',
             'testMultiple', 'testMultipleTLS',
             'testFilter', 'testPostCommand']

rfiles = None

TIMEOUT_FACTOR = 1.0


if sys.version_info[0] >= 3:
    coding = {"encoding": "ascii"}
else:
    coding = {}


class TriggerError(Exception):
    pass

class FileTransferError(Exception):
    pass

def global_log(name, msg, timestamp=True):
    if not name:
        name = sys.argv[0]
    out = name + ": "
    if timestamp:
        out += datetime.datetime.now().strftime("%b %d %H:%M:%S ")
    out += msg
    if out[-1] != "\n":
        out += "\n"
    for dest in LOG_OUTPUT:
        dest.write(out)
        dest.flush()

def t(dur):
    return int(dur * TIMEOUT_FACTOR)

def setup():
    global rfiles
    rfiles = []
    top_tests = os.path.dirname(FILE_LIST_FILE)
    file_list = open(FILE_LIST_FILE, "r", 1)
    for line in file_list:
        (path, size, md5) = line.split()
        path = os.path.join(top_tests, path)
        # empty string is placeholder for SHA1 digest
        rfiles.append((path, (int(size), "", md5)))

def teardown():
    pass

def tls_supported():
    return eval(config_vars.get('SK_ENABLE_GNUTLS', None))

def ipv6_supported():
    return eval(config_vars.get('SK_ENABLE_INET6_NETWORKING', None))

trigger_id = 0

def trigger(*specs, **kwd):
    global trigger_id
    trigger_id += 1
    pid = kwd.get('pid', True)
    first = kwd.get('first', False)
    pipes = {}
    count = len(specs)
    retval = []
    for daemon, timeout, match in specs:
        daemon.dump(('trigger',
                     {'pid': pid, 'id': trigger_id,
                      'match': match, 'timeout': t(timeout)}))
        pipes[daemon.pipe] = (daemon, timeout, match, len(retval))
        retval.append(None)
    while count:
        (readers, writers, x) = select.select(pipes.keys(), [], [])
        for reader in readers:
            data = pipes[reader]
            try:
                rv = data[0].load()
            except EOFError:
                raise TriggerError(data[:3])
            if rv[0] != 'trigger':
                # Trigger failed
                raise TriggerError(data[:3])
            if rv[1] != trigger_id:
                continue
            if rv[2] == False:
                raise TriggerError(data[:3])
            count -= 1
            retval[data[3]] = rv[3]
            del pipes[reader]
            if first:
                return retval
    return retval

def check_dh(*daemons):
    # Wait for Diffie-Hellman parameters to be generated
    trigger(*((x, 10, "Generating Diffie-Hellman") for x in daemons))
    trigger(*((x, 300, "Finished generating Diffie-Hellman")
              for x in daemons))

def create_random_file(suffix="", prefix="random", dir=None, size=(0, 0)):
    (handle, path) = tempfile.mkstemp(suffix, prefix, dir)
    f = os.fdopen(handle, "w")
    numbytes = random.randint(size[0], size[1])
    totalbytes = numbytes
    #checksum_sha = sha1_new()
    checksum_md5 = md5_new()
    while numbytes:
        length = min(numbytes, CHUNKSIZE)
        try:
            bytes = os.urandom(length)
        except NotImplementedError:
            bytes = ''.join(chr(random.getrandbits(8))
                            for x in range(0, length))
        f.write(bytes)
        #checksum_sha.update(bytes)
        checksum_md5.update(bytes)
        numbytes -= length
    f.close()
    # empty string is placeholder for SHA1 digest
    return (path, (totalbytes, "", checksum_md5.hexdigest()))

def checksum_file(path):
    f = open(path, 'rb')
    #checksum_sha = sha1_new()
    checksum_md5 = md5_new()
    size = os.fstat(f.fileno())[stat.ST_SIZE]
    data = f.read(CHUNKSIZE)
    while data:
        #checksum_sha.update(data)
        checksum_md5.update(data)
        data = f.read(CHUNKSIZE)
    f.close()
    # empty string is placeholder for SHA1 digest
    return (size, "", checksum_md5.hexdigest())


sconv = "L"
slen  = struct.calcsize(sconv)


class Daemon(Dirobject):

    def __init__(self, name=None, log_level="info", prog_env=None,
                 verbose=True, **kwds):
        global global_int
        Dirobject.__init__(self, **kwds)
        if name:
            self.name = name
        else:
            self.name = type(self).__name__ + str(global_int)
            global_int += 1
        self.process = None
        self.logdata = []
        self.log_level = log_level
        self._prog_env = prog_env
        self.daemon = True
        self.verbose = verbose
        self.pipe = None
        self.pid = None
        self.trigger = None
        self.timeout = None
        self.channels = []
        self.pending_line = None

    def printv(self, *args):
        if self.pid is None or self.pid != 0:
            me = "parent"
        else:
            me = "child"
        if self.verbose:
            global_log(self.name, me + ("[%s]:" % os.getpid()) +
                       ' '.join(str(x) for x in args))

    def get_executable(self):
        return os.environ.get(self._prog_env,
                              os.path.join(".", self.exe_name))

    def get_args(self):
        args = self.get_executable().split()
        args.extend([ '--no-daemon',
                      '--log-dest', 'stderr',
                      '--log-level', self.log_level])
        return args

    def init(self):
        pass

    def log_verbose(self, msg):
        if self.verbose:
            global_log(self.name, msg)

    def _handle_log(self, fd):
        # 'fd' is the log of the process, which supports non-blocking
        # reads.  read from the log until there is thing left to read,
        # so that select() on 'fd' will work correctly
        got_line = False
        try:
            # when no-more-data, following returns empty string in
            # Python3, but throws IOError [EAGAIN] in Python2
            for line in fd:
                line = str(line, **coding)
                # handle reading a partial line last time and this time
                if self.pending_line:
                    line = self.pending_line + line
                    self.pending_line = None
                if line[-1] != "\n":
                    self.pending_line = line
                    break
                got_line = True
                self.logdata.append(line)
                global_log(self.name, line, timestamp=False)
                if self.trigger:
                    match = self.trigger['re'].search(line)
                    if match:
                        self.log_verbose(
                            "Trigger fired for %s" % self.trigger['match'])
                        self.dump(('trigger', self.trigger['id'], True, line))
                        self.timeout = None
                        self.trigger = None
        except IOError:
            pass
        if not got_line:
            return False
        return True

    def _handle_parent(self):
        retval = False
        request = self.load()
        # self.log_verbose("Handling %s" % request)
        if request[0] == 'stop':
            self._stop()
        if request[0] == 'start':
            if self.process is not None:
                self.process.poll()
                if self.process.returncode is None:
                    raise RuntimeError()
            self._start()
        elif request[0] == 'kill':
            self._kill()
        elif request[0] == 'end':
            self._end()
            self.dump(("stopped", self.process.returncode))
        elif request[0] == 'exit':
            try:
                self.pipe.close()
            except:
                pass
            os._exit(0)
        elif request[0] == 'trigger':
            # search previously captured output from the application
            self.trigger = request[1]
            if self.trigger['pid']:
                regexp = re.compile(r"\[%s\].*%s" %
                                    (self.process.pid, self.trigger['match']))
            else:
                regexp = re.compile(self.trigger['match'])
            for line in self.logdata:
                match = regexp.search(line)
                if match:
                    self.log_verbose(
                        "Trigger fired for %s" % self.trigger['match'])
                    self.dump(('trigger', self.trigger['id'], True, line))
                    self.trigger = None
                    return retval
            self.trigger['re'] = regexp
            self.timeout = time.time() + self.trigger['timeout']
        return retval

    def _child(self):
        while self.channels:
            # channels contains a socket to the parent and to the
            # stderr of the application's process
            (readers, writers, x) = select.select(self.channels, [], [], 1)
            if self.process is not None and self.process.stderr in readers:
                rv = self._handle_log(self.process.stderr)
                if not rv:
                    self.channels.remove(self.process.stderr)
            if self.pipe in readers:
                if self._handle_parent():
                    try:
                        self.pipe.close()
                    except:
                        pass
                    self.channels.remove(self.pipe)
            if self.timeout is not None and time.time() > self.timeout:
                self.log_verbose(
                    "Trigger timed out after %s seconds: %s" %
                    (self.trigger['timeout'], self.trigger['match']))
                self.dump(('trigger', self.trigger['id'], False))
                self.timeout = None
                self.trigger = None

    def expect(self, cmd):
        try:
            while True:
                rv = self.load()
                # self.log_verbose("Retrieved %s" % rv)
                if cmd == rv[0]:
                    break
            return rv
        except EOFError:
            if cmd in ['stop', 'kill', 'end']:
                return ('stopped', None)
            raise

    def start(self):
        if self.pid is not None:
            self.dump(('start',))
            self.expect('start')
            return None
        pipes = socket.socketpair()
        self.pid = os.fork()
        if self.pid != 0:
            pipes[1].close()
            self.pipe = pipes[0]
            self.dump(('start',))
            self.expect('start')
            return None
        try:
            pipes[0].close()
            self.pipe = pipes[1]
            self.channels = [self.pipe]
            self._child()
        except:
            traceback.print_exc()
            self._kill()
        finally:
            try:
                self.pipe.close()
            except:
                pass
            os._exit(0)

    def _start(self):
        if self.process is not None and self.process.stderr in self.channels:
            self.channels.remove(self.process.stderr)
        # work around issue #11459 in python 3.1.[0-3], 3.2.0 (where a
        # process is line buffered despite bufsize=0) by making the
        # buffer large, making the stream non-blocking, and getting
        # everything available from the stream when we read
        self.process = subprocess.Popen(self.get_args(), bufsize = -1,
                                        stderr=subprocess.PIPE)
        fcntl.fcntl(self.process.stderr, fcntl.F_SETFL,
                    (os.O_NONBLOCK
                     | fcntl.fcntl(self.process.stderr, fcntl.F_GETFL)))
        self.channels.append(self.process.stderr)
        self.dump(('start',))

    def dump(self, arg):
        value = conv.dumps(arg).encode('ascii')
        data = struct.pack(sconv, len(value)) + value
        try:
            self.pipe.sendall(data)
        except IOError:
            pass

    def load(self):
        rv = self.pipe.recv(slen)
        if len(rv) != slen:
            raise RuntimeError
        (length,) = struct.unpack(sconv, rv)
        value = b""
        while len(value) != length:
            value += self.pipe.recv(length)
        retval = conv.loads(value.decode('ascii'))
        return retval

    def kill(self):
        if self.pid is None:
            return None
        self.dump(('kill',))
        self.expect('kill')

    def stop(self):
        if self.pid is None:
            return None
        self.dump(('stop',))
        self.expect('stop')

    def end(self):
        if self.pid is None:
            return None
        self.dump(('end',))
        rv = self.expect('stopped')[1]
        if rv is not None:
            if rv >= 0:
                self.log_verbose("Exited with status %s" % rv)
            else:
                self.log_verbose("Exited with signal %s" % (-rv))
        return rv

    def exit(self):
        self.end()
        if self.pid is None:
            return None
        self.dump(('exit',))
        try:
            os.waitpid(self.pid, 0)
        except OSError:
            pass

    def _kill(self):
        if self.process is not None and self.process.returncode is None:
            try:
                self.log_verbose("Sending SIGKILL")
                os.kill(self.process.pid, signal.SIGKILL)
            except OSError:
                pass
        self.dump(('kill',))

    def _stop(self):
        if self.process is not None and self.process.returncode is None:
            try:
                self.log_verbose("Sending SIGTERM")
                os.kill(self.process.pid, signal.SIGTERM)
            except OSError:
                pass
        self.dump(('stop',))

    def _end(self):
        target = time.time() + KILL_DELAY
        self.process.poll()
        while self.process.returncode is None and time.time() < target:
            self.process.poll()
            time.sleep(1)
        if self.process.returncode is not None:
            while self._handle_log(self.process.stderr):
                pass
            return True
        self._kill()
        self.process.poll()
        while self.process.returncode is None:
            self.process.poll()
            time.sleep(1)
        while self._handle_log(self.process.stderr):
            pass
        return False


class Sndrcv_base(Daemon):

    def __init__(self, name=None, **kwds):
        Daemon.__init__(self, name, **kwds)
        self.mode = "client"
        self.listen = None
        self.port = None
        self.clients = list()
        self.servers = list()
        self.ca_cert = None
        self.ca_key = None
        self.cert = None

    def create_cert(self):
        self.cert = generate_signed_cert(self.basedir,
                                         (self.ca_key, self.ca_cert),
                                         "key.pem", "key.p12")

    def init(self):
        Daemon.init(self)
        if self.ca_cert:
            self.dirs.append("cert")
        self.create_dirs()
        if self.ca_cert:
            self.create_cert()

    def get_args(self):
        args = Daemon.get_args(self)
        args += ['--mode', self.mode,
                 '--identifier', self.name]
        if self.ca_cert:
            args += ['--tls-ca', os.path.abspath(self.ca_cert),
                     '--tls-pkcs12', os.path.abspath(self.cert)]
        if self.mode == "server":
            if self.listen is not None:
                args += ['--server-port', "%s:%s" % (self.listen, self.port)]
            else:
                args += ['--server-port', str(self.port)]
            for client in self.clients:
                 args += ['--client-ident', client]
        else:
            for (ident, addr, port) in self.servers:
                args += ['--server-address',
                         ':'.join((ident, addr, str(port)))]
        return args

    def _check_file(self, dir, finfo):
        (path, (size, ck_sha, ck_md5)) = finfo
        path = os.path.join(self.dirname[dir], os.path.basename(path))
        if not os.path.exists(path):
            return ("Does not exist", path)
        (nsize, ck2_sha, ck2_md5) = checksum_file(path)
        if nsize != size:
            return ("Size mismatch (%s != %s)" % (size, nsize), path)
        if ck2_sha != ck_sha:
            return ("SHA mismatch (%s != %s)" % (ck_sha, ck2_sha), path)
        if ck2_md5 != ck_md5:
            return ("MD5 mismatch (%s != %s)" % (ck_md5, ck2_md5), path)
        return (None, path)


class Rwsender(Sndrcv_base):

    def __init__(self, name=None, polling_interval=5, filters=[],
                 overwrite=None, log_level=None, **kwds):
        if log_level is None:
            log_level = LOG_LEVEL
        if overwrite is None:
            overwrite = OVERWRITE
        Sndrcv_base.__init__(self, name, overwrite=overwrite,
                             log_level=log_level, prog_env="RWSENDER", **kwds)
        self.exe_name = "rwsender"
        self.filters = filters
        self.polling_interval = polling_interval
        self.dirs = ["in", "proc", "error"]

    def get_args(self):
        args = Sndrcv_base.get_args(self)
        args += ['--incoming-directory', os.path.abspath(self.dirname["in"]),
                 '--processing-directory',
                 os.path.abspath(self.dirname["proc"]),
                 '--error-directory', os.path.abspath(self.dirname["error"]),
                 '--polling-interval', str(self.polling_interval)]
        for ident, regexp in self.filters:
            args.extend(["--filter", ident + ':' + regexp])
        return args

    def send_random_file(self, suffix="", prefix="random", size=(0, 0)):
        return create_random_file(suffix = suffix, prefix = prefix,
                                  dir = self.dirname["in"], size = size)

    def send_files(self, files):
        for f, data in files:
            shutil.copy(f, self.dirname["in"])

    def check_error(self, data):
        return self._check_file("error", data)


class Rwreceiver(Sndrcv_base):

    def __init__(self, name=None, post_command=None,
                 overwrite=None, log_level=None, **kwds):
        if log_level is None:
            log_level = LOG_LEVEL
        if overwrite is None:
            overwrite = OVERWRITE
        Sndrcv_base.__init__(self, name, overwrite=overwrite,
                             log_level=log_level, prog_env="RWRECEIVER", **kwds)
        self.exe_name = "rwreceiver"
        self.dirs = ["dest"]
        self.post_command = post_command

    def get_args(self):
        args = Sndrcv_base.get_args(self)
        args += ['--destination-directory',
                 os.path.abspath(self.dirname["dest"])]
        if self.post_command:
            args += ['--post-command', self.post_command]
        return args

    def check_sent(self, data):
        return self._check_file("dest", data)


class System(Dirobject):

    def __init__(self):
        Dirobject.__init__(self)
        self.create_dirs()
        self.client_type = None
        self.server_type = None
        self.clients = set()
        self.servers = set()
        self.ca_cert = None
        self.ca_key = None

    def create_ca_cert(self):
        self.ca_key, self.ca_cert = generate_ca_cert(self.basedir,
                                                     'ca_cert.pem')

    def connect(self, clients, servers, tls=False, hostname=None):
        if tls:
            self.create_ca_cert()
        if hostname is None:
            hostname = os.environ.get("SK_TESTS_SENDRCV_HOSTNAME")
            if hostname is None:
                hostname = "localhost"
        if isinstance(clients, Sndrcv_base):
            clients = [clients]
        if isinstance(servers, Sndrcv_base):
            servers = [servers]
        for server in servers:
            server.listen = hostname
        for client in clients:
            for server in servers:
                self._connect(client, server, tls, hostname)

    def _connect(self, client, server, tls, hostname):
        if not isinstance(client, Sndrcv_base):
            raise ValueError("Can only connect rwsenders and rwreceivers")
        if not self.client_type:
            if isinstance(client, Rwsender):
                self.client_type = Rwsender
                self.server_type = Rwreceiver
            else:
                self.client_type = Rwreceiver
                self.server_type = Rwsender
        if not isinstance(client, self.client_type):
            raise ValueError("Client must be of type %s" %
                               self.client_type.__name__)
        if not isinstance(server, self.server_type):
            raise ValueError("Server must be of type %s" %
                               self.server_type.__name__)
        client.mode = "client"
        server.mode = "server"

        if server.port is None:
            server.port = get_ephemeral_port()

        client.servers.append((server.name, hostname, server.port))
        server.clients.append(client.name)

        self.clients.add(client)
        self.servers.add(server)

        if tls:
            client.ca_cert = self.ca_cert
            server.ca_cert = self.ca_cert
            client.ca_key = self.ca_key
            server.ca_key = self.ca_key

    def _forall(self, call, which, *args, **kwds):
        if which == "clients":
            it = self.clients
        elif which == "servers":
            it = self.servers
        else:
            it = itertools.chain(self.clients, self.servers)
        return [getattr(x, call)(*args, **kwds) for x in it]

    def start(self, which = None):
        self._forall("init", which)
        self._forall("start", which)

    def end(self, which = None, noremove=False):
        self._forall("exit", which)
        if not noremove:
            self.remove_basedir()

    def stop(self, which = None):
        self._forall("stop", which)


#def Sender(**kwds):
#    return Rwsender(overwrite=OVERWRITE, log_level=LOG_LEVEL, **kwds)
#
#def Receiver(**kwds):
#    return Rwreceiver(overwrite=OVERWRITE, log_level=LOG_LEVEL, **kwds)

def _testConnectAndClose(tls=False, hostname="localhost"):
    if tls and not tls_supported():
        return None
    reset_all_certs_and_keys()
    s1 = Rwsender()
    r1 = Rwreceiver()
    sy = System()
    try:
        sy.connect(s1, r1, tls=tls, hostname=hostname)
        sy.start()
        if tls:
            check_dh(s1, r1)
        trigger((s1, 25, "Connected to remote %s" % r1.name),
                (r1, 25, "Connected to remote %s" % s1.name))
        sy.stop()
        trigger((s1, 20, "Finished shutting down"),
                (r1, 20, "Finished shutting down"))
        trigger((s1, 25, "Stopped logging"),
                (r1, 25, "Stopped logging"))
    except:
        traceback.print_exc()
        sy.stop()
        raise
    finally:
        sy.end(noremove=NO_REMOVE)

def testConnectOnlyIPv4Addr():
    """
    Test to see if we can start a sender/receiver pair, that they
    connect, and that they shut down properly.
    """
    _testConnectAndClose(hostname="127.0.0.1")

def testConnectOnlyHostname():
    """
    Test to see if we can start a sender/receiver pair, that they
    connect, and that they shut down properly.
    """
    _testConnectAndClose()

def testConnectOnlyTLS():
    """
    Test to see if we can start a sender/receiver pair using TLS,
    that they connect, and that they shut down properly.
    """
    _testConnectAndClose(tls=True)

def testConnectOnlyIPv6Addr():
    """
    Test to see if we can start a sender/receiver pair, that they
    connect, and that they shut down properly.
    """
    if not ipv6_supported():
        return None
    _testConnectAndClose(hostname="[::1]")

def _testSendRcv(tls=False,
                 sender_client=True,
                 stop_sender=False,
                 kill=False):
    if tls and not tls_supported():
        return None
    global rfiles
    reset_all_certs_and_keys()
    s1 = Rwsender()
    r1 = Rwreceiver()
    if stop_sender:
        if kill:
            stop = s1.kill
        else:
            stop = s1.stop
        end = s1.end
        start = s1.start
        stopped = s1
    else:
        if kill:
            stop = r1.kill
        else:
            stop = r1.stop
        end = r1.end
        start = r1.start

        stopped = r1
    s1.create_dirs()
    s1.send_files(rfiles)
    sy = System()
    try:
        if sender_client:
            sy.connect(s1, r1, tls=tls)
        else:
            sy.connect(r1, s1, tls=tls)
        sy.start()
        if tls:
            check_dh(s1, r1)

        trigger((s1, 75, "Connected to remote %s" % r1.name),
                (r1, 75, "Connected to remote %s" % s1.name))
        trigger((s1, 40, "Succeeded sending .* to %s" % r1.name))
        stop()
        if not kill:
            trigger((stopped, 25, "Stopped logging"))
        end()
        start()
        if tls:
            check_dh(stopped)
        trigger((s1, 75, "Connected to remote %s" % r1.name),
                (r1, 75, "Connected to remote %s" % s1.name))
        try:
            for path, data in rfiles:
                base = os.path.basename(path)
                data = {"name": re.escape(base),
                        "rname": r1.name, "sname": s1.name}
                trigger((s1, 40,
                         ("Succeeded sending .*/%(name)s to %(rname)s|"
                          "Remote side %(rname)s rejected .*/%(name)s")
                         % data),
                        (r1, 40,
                         "Finished receiving from %(sname)s: %(name)s" % data),
                        pid=False, first=True)
        except TriggerError:
            pass
        for f in rfiles:
            (error, path) = r1.check_sent(f)
            if error:
                global_log(False, ("Error receiving %s: %s" %
                                   (os.path.basename(f[0]), error)))
                raise FileTransferError()
        sy.stop()
        trigger((s1, 25, "Stopped logging"),
                (r1, 25, "Stopped logging"))
    except KeyboardInterrupt:
        global_log(False, "%s: Interrupted by C-c", os.getpid())
        traceback.print_exc()
        sy.stop()
        raise
    except:
        traceback.print_exc()
        sy.stop()
        raise
    finally:
        sy.end(noremove=NO_REMOVE)


def testSendRcvStopReceiverServer():
    """
    Test a sender/receiver connection, with receiver as server,
    sending files.  Midway the connection is terminated by
    stopping the sender.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=True, stop_sender=False, kill=False)

def testSendRcvStopSenderServer():
    """
    Test a sender/receiver connection, with sender as server,
    sending files.  Midway the connection is terminated by
    stopping the sender.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=False, stop_sender=True, kill=False)

def testSendRcvStopReceiverClient():
    """
    Test a sender/receiver connection, with sender as server,
    sending files.  Midway the connection is terminated by
    stopping the receiver.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=False, stop_sender=False, kill=False)

def testSendRcvStopSenderClient():
    """
    Test a sender/receiver connection, with receiver as server,
    sending files.  Midway the connection is terminated by
    stopping the sender.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=True, stop_sender=True, kill=False)

def testSendRcvKillReceiverServer():
    """
    Test a sender/receiver connection, with receiver as server,
    sending files.  Midway the connection is terminated by
    killing the sender.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=True, stop_sender=False, kill=True)

def testSendRcvKillSenderServer():
    """
    Test a sender/receiver connection, with sender as server,
    sending files.  Midway the connection is terminated by
    killing the sender.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=False, stop_sender=True, kill=True)

def testSendRcvKillReceiverClient():
    """
    Test a sender/receiver connection, with sender as server,
    sending files.  Midway the connection is terminated by
    killing the receiver.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=False, stop_sender=False, kill=True)

def testSendRcvKillSenderClient():
    """
    Test a sender/receiver connection, with receiver as server,
    sending files.  Midway the connection is terminated by
    killing the sender.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=True, stop_sender=True, kill=True)


def testSendRcvStopReceiverServerTLS():
    """
    Test a sender/receiver connection, with receiver as server,
    sending files.  Midway the connection is terminated by
    stopping the sender.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=True, stop_sender=False, kill=False, tls=True)

def testSendRcvStopSenderServerTLS():
    """
    Test a sender/receiver connection, with sender as server,
    sending files.  Midway the connection is terminated by
    stopping the sender.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=False, stop_sender=True, kill=False, tls=True)

def testSendRcvStopReceiverClientTLS():
    """
    Test a sender/receiver connection, with sender as server,
    sending files.  Midway the connection is terminated by
    stopping the receiver.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=False, stop_sender=False, kill=False, tls=True)

def testSendRcvStopSenderClientTLS():
    """
    Test a sender/receiver connection, with receiver as server,
    sending files.  Midway the connection is terminated by
    stopping the sender.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=True, stop_sender=True, kill=False, tls=True)

def testSendRcvKillReceiverServerTLS():
    """
    Test a sender/receiver connection, with receiver as server,
    sending files.  Midway the connection is terminated by
    killing the sender.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=True, stop_sender=False, kill=True, tls=True)

def testSendRcvKillSenderServerTLS():
    """
    Test a sender/receiver connection, with sender as server,
    sending files.  Midway the connection is terminated by
    killing the sender.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=False, stop_sender=True, kill=True, tls=True)

def testSendRcvKillReceiverClientTLS():
    """
    Test a sender/receiver connection, with sender as server,
    sending files.  Midway the connection is terminated by
    killing the receiver.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=False, stop_sender=False, kill=True, tls=True)

def testSendRcvKillSenderClientTLS():
    """
    Test a sender/receiver connection, with receiver as server,
    sending files.  Midway the connection is terminated by
    killing the sender.  The connection is restarted and resumed.
    """
    _testSendRcv(sender_client=True, stop_sender=True, kill=True, tls=True)


def _testMultiple(tls=False):
    global rfiles
    if tls and not tls_supported():
        return None
    reset_all_certs_and_keys()
    s1 = Rwsender()
    s2 = Rwsender()
    r1 = Rwreceiver()
    r2 = Rwreceiver()
    sy = System()
    try:
        sy.connect([r1, r2], [s1, s2], tls=tls)
        sy.start()
        if tls:
            check_dh(s1, r1, s2, r2)

        trigger((s1, 70, "Connected to remote %s" % r1.name),
                (r1, 70, "Connected to remote %s" % s1.name),
                (r2, 70, "Connected to remote %s" % s1.name),
                (s2, 70, "Connected to remote %s" % r1.name))
        trigger((s1, 70, "Connected to remote %s" % r2.name),
                (r1, 70, "Connected to remote %s" % s2.name),
                (s2, 70, "Connected to remote %s" % r2.name),
                (r2, 70, "Connected to remote %s" % s2.name))

        filea = rfiles[0]
        fileb = rfiles[1]
        s1.send_files([filea])
        s2.send_files([fileb])
        params = {"filea": re.escape(os.path.basename(filea[0])),
                  "fileb": re.escape(os.path.basename(fileb[0])),
                  "rnamec": r1.name, "rnamed": r2.name}

        trigger((s1, 40,
                 "Succeeded sending .*/%(filea)s to %(rnamec)s" % params),
                (s2, 40,
                 "Succeeded sending .*/%(fileb)s to %(rnamec)s" % params))
        trigger((s1, 40,
                 "Succeeded sending .*/%(filea)s to %(rnamed)s" % params),
                (s2, 40,
                 "Succeeded sending .*/%(fileb)s to %(rnamed)s" % params))
        for f in [filea, fileb]:
            for r in [r1, r2]:
                (error, path) = r.check_sent(f)
                if error:
                    global_log(False, ("Error receiving %s: %s" %
                                       (os.path.basename(f[0]), error)))
                    raise FileTransferError()
        sy.stop()
        trigger((s1, 25, "Stopped logging"),
                (r1, 25, "Stopped logging"),
                (s2, 25, "Stopped logging"),
                (r2, 25, "Stopped logging"))
    except:
        traceback.print_exc()
        sy.stop()
        raise
    finally:
        sy.end(noremove=NO_REMOVE)

def testMultiple():
    """
    Test two senders connected to two receivers.  Each sender
    sends a file to both receivers.
    """
    _testMultiple()

def testMultipleTLS():
    """
    Test two senders connected to two receivers via TLS.  Each
    sender sends a file to both receivers.
    """
    _testMultiple(tls=True)


def _testFilter(tls=False):
    global rfiles
    if tls and not tls_supported():
        return None
    reset_all_certs_and_keys()
    r1 = Rwreceiver()
    r2 = Rwreceiver()
    s1 = Rwsender(filters=[(r1.name, "[a-g]$"), (r2.name, "[d-j]$")])
    sy = System()
    try:
        sy.connect([r1, r2], s1, tls=tls)
        sy.start()
        if tls:
            check_dh(s1, r1, r2)
        trigger((s1, 70, "Connected to remote %s" % r1.name),
                (r1, 70, "Connected to remote %s" % s1.name),
                (r2, 70, "Connected to remote %s" % s1.name))
        trigger((s1, 70, "Connected to remote %s" % r2.name))

        s1.send_files(rfiles)
        cfiles = [x for x in rfiles if 'a' <= x[0][-1] <= 'g']
        dfiles = [x for x in rfiles if 'd' <= x[0][-1] <= 'j']
        for (f, data) in cfiles:
            trigger((s1, 25,
                     "Succeeded sending .*/%(file)s to %(name)s"
                     % {"file": re.escape(os.path.basename(f)),
                        "name" : r1.name}))
        for (f, data) in dfiles:
            trigger((s1, 25,
                     "Succeeded sending .*/%(file)s to %(name)s"
                     % {"file": re.escape(os.path.basename(f)),
                        "name" : r2.name}))
        for f in cfiles:
            (error, path) = r1.check_sent(f)
            if error:
                global_log(False, ("Error receiving %s: %s" %
                                   (os.path.basename(f[0]), error)))
                raise FileTransferError()
        for f in dfiles:
            (error, path) = r2.check_sent(f)
            if error:
                global_log(False, ("Error receiving %s: %s" %
                                   (os.path.basename(f[0]), error)))
        cset = set(cfiles)
        dset = set(dfiles)
        for f in cset - dset:
            (error, path) = r2.check_sent(f)
            if not error:
                global_log(False, ("Unexpectedly received file %s" %
                                   os.path.basename(f[0])))
                raise FileTransferError()
        for f in dset - cset:
            (error, path) = r1.check_sent(f)
            if not error:
                global_log(False, ("Unexpectedly received file %s" %
                                   os.path.basename(f[0])))
                raise FileTransferError()
        sy.stop()
        trigger((s1, 25, "Stopped logging"),
                (r1, 25, "Stopped logging"),
                (r2, 25, "Stopped logging"))
    except:
        traceback.print_exc()
        sy.stop()
        raise
    finally:
        sy.end(noremove=NO_REMOVE)

def testFilter():
    """
    Test filtering with a sender and two receivers.  Using
    filters, some files get sent to receiver A, some to receiver
    B, and some to both.
    """
    _testFilter()


def testPostCommand():
    """
    Test the post command option.
    """
    global rfiles
    if srcdir:
        cmddir = os.path.join(srcdir, "tests")
    else:
        cmddir = os.path.join(".", "tests")
    command = os.path.join(cmddir, "post-command.sh")
    post_command = command + " %I %s"
    s1 = Rwsender()
    r1 = Rwreceiver(post_command=post_command)
    s1.create_dirs()
    s1.send_files(rfiles)
    sy = System()
    try:
        sy.connect(s1, r1)
        sy.start()
        trigger((s1, 70, "Connected to remote %s" % r1.name),
                (r1, 70, "Connected to remote %s" % s1.name))
        for path, data in rfiles:
            trigger((r1, 40,
                     ("Post command: Ident: %(sname)s  "
                      "Filename: .*/%(file)s") %
                     {"file": re.escape(os.path.basename(path)),
                      "sname": s1.name}), pid=False)
        sy.stop()
        trigger((s1, 25, "Stopped logging"),
                (r1, 25, "Stopped logging"))
    except:
        traceback.print_exc()
        sy.stop()
        raise
    finally:
        sy.end(noremove=NO_REMOVE)


if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option("--verbose", action="store_true", dest="verbose",
                      default=False)
    parser.add_option("--overwrite-dirs", action="store_true",
                      dest="overwrite", default=False)
    parser.add_option("--save-output", action="store_true", dest="save_output",
                      default=False)
    parser.add_option("--log-level", action="store", type="string",
                      dest="log_level", default="info")
    parser.add_option("--log-output-to", action="store", type="string",
                      dest="log_output", default=None)
    parser.add_option("--file-list-file", action="store", type="string",
                      dest="file_list_file", default=None)
    parser.add_option("--print-test-names", action="store_true",
                      dest="print_test_names", default=False)
    parser.add_option("--timeout-factor", action="store", type="float",
                      dest="timeout_factor", default = 1.0)
    (options, args) = parser.parse_args()

    if options.print_test_names:
        print_test_names()
        sys.exit()

    if not options.file_list_file:
        sys.exit("The --file-list-file switch is required when running tests")

    FILE_LIST_FILE = options.file_list_file
    OVERWRITE = options.overwrite
    LOG_LEVEL = options.log_level
    NO_REMOVE = options.save_output

    (fd, path) = tempfile.mkstemp(".log", "sendrcv-", None)
    LOG_OUTPUT.append(os.fdopen(fd, "a"))
    if options.verbose:
        LOG_OUTPUT.append(sys.stdout)
    if options.log_output:
        LOG_OUTPUT.append(open(options.log_output, "a"))

    TIMEOUT_FACTOR = options.timeout_factor

    if not args:
        args = ALL_TESTS

    setup()
    retval = 1

    try:
        for x in args:
            locals()[x]()
    except SystemExit:
        raise
    finally:
        teardown()


