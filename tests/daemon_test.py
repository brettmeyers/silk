#######################################################################
# Copyright (C) 2009-2015 by Carnegie Mellon University.
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
# $SiLK: daemon_test.py b7b8edebba12 2015-01-05 18:05:21Z mthomas $
#######################################################################
from __future__ import print_function
import numbers
import os
import os.path
import select
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import threading

from config_vars import config_vars


#V5PDU_LEN  = 1464
TCPBUF     = 2048
LINEBUF    = 1024

if sys.version_info[0] >= 3:
    coding = {"encoding": "latin_1"}
else:
    coding = {}

def string_write(f, s):
    return f.write(bytes(s, **coding))

class TimedReadline(object):
    def __init__(self, fd):
        self.buf = ""
        if isinstance(fd, numbers.Integral):
            self.fd = fd
        else:
            self.fd = fd.fileno()

    def __call__(self, timeout):
        while True:
            x = self.buf.find('\n')
            if x >= 0:
                retval = self.buf[:x+1]
                self.buf = self.buf[x+1:]
                return retval
            (r, w, x) = select.select([self.fd], [], [], timeout)
            if r:
                more = os.read(self.fd, LINEBUF)
                if more:
                    self.buf += more.decode('latin_1')
                else:
                    return None
            else:
                return ""

class Dirobject(object):

    def __init__(self, overwrite=False, basedir=None):
        self.dirs = list()
        self.dirname = dict()
        self.basedir = basedir
        self.dirs_created = False
        self.overwrite = overwrite

    def create_basedir(self):
        if self.basedir:
            if not os.path.exists(self.basedir):
                os.mkdir(self.basedir)
        else:
            self.basedir = tempfile.mkdtemp()

    def remove_basedir(self):
        if self.basedir and self.dirs_created:
            shutil.rmtree(self.basedir)
            self.dirs_created = False

    def create_dirs(self):
        if not self.dirs_created:
            self.create_basedir()
            for name in self.dirs:
                self.dirname[name] = os.path.abspath(
                    os.path.join(self.basedir, name))
                if os.path.exists(self.dirname[name]):
                    if self.overwrite:
                        shutil.rmtree(self.dirname[name])
                        os.mkdir(self.dirname[name])
                else:
                    os.mkdir(self.dirname[name])
            self.dirs_created = True

    def get_path(self, name, path):
        return os.path.join(self.dirname[name], path)


class PduSender(object):
    def __init__(self, max_recs, port, log, address="localhost"):
        self._port = port
        self._max_recs = max_recs
        self._log = log
        self._address = ("[%s]" % address)
        self.process = None

    def start(self):
        prog = os.path.join(os.environ["top_srcdir"], "tests", "make-data.pl")
        args = [config_vars["PERL"], prog,
                "--pdu-network", self._address + ":" + str(self._port),
                "--max-records", str(self._max_recs)]
        self._log("Starting: %s" % args)
        self.process = subprocess.Popen(args)
        return self.process

    def stop(self):
        if self.process is None:
            return None
        self.process.poll()
        if self.process.returncode is None:
            try:
                os.kill(self.process.pid, signal.SIGTERM)
            except OSError:
                pass
        return self.process.returncode


class TcpSender(object):
    def __init__(self, file, port, log, address="localhost"):
        self._file = file
        self._port = port
        self._log = log
        self._address = address
        self._running = False

    def start(self):
        thread = threading.Thread(target = self.go)
        thread.daemon = True
        thread.start()

    def go(self):
        self._running = True
        sock = None
        # Try each address until we connect to one; no need to report
        # errors here
        for res in socket.getaddrinfo(self._address, self._port,
                                      socket.AF_UNSPEC, socket.SOCK_STREAM):
            af, socktype, proto, canonname, sa = res
            try:
                sock = socket.socket(af, socktype, proto)
            except socket.error:
                sock = None
                continue
            try:
                sock.connect(sa)
            except socket.error:
                sock.close()
                sock = None
                continue
            break
        if sock is None:
            self._log("Could not open connection to [%s]:%d" %
                      (self._address, self._port))
            sys.exit(1)
        self._log("Connected to [%s]:%d" % (self._address, self._port))
        sock.settimeout(1)
        # Send the data
        while self._running:
            pdu = self._file.read(TCPBUF)
            if not pdu:
                self._running = False
                continue
            count = len(pdu)
            while self._running and count:
                try:
                    num_sent = sock.send(pdu)
                    pdu = pdu[num_sent:]
                    count -= num_sent
                except socket.timeout:
                    pass
                except socket.error as msg:
                    if isinstance(msg, tuple):
                        errmsg = msg[1]
                    else:
                        errmsg = msg
                    self._log("Error sending to [%s]:%d: %s" %
                              (self._address, self._port, errmsg))
                    self._running = False
        # Done
        sock.close()

    def stop(self):
        self._running = False

def get_ephemeral_port():
    sock = socket.socket()
    sock.bind(("", 0))
    (addr, port) = sock.getsockname()
    sock.close()
    return port
