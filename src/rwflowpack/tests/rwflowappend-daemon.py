#!/usr/bin/env python
#######################################################################
# Copyright (C) 2009-2014 by Carnegie Mellon University.
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
# $SiLK: rwflowappend-daemon.py 39e8161c33bb 2014-03-31 18:57:22Z mthomas $
#######################################################################
from __future__ import print_function
import optparse
import subprocess
import signal
import re
import time
import traceback
import shutil
import sys
import os
import os.path

from daemon_test import Dirobject, TimedReadline, string_write

VERBOSE = False
logfile = None

def base_log(*args):
    global VERBOSE
    global logfile

    out = ' '.join(str(x) for x in args)
    if out[-1] != '\n':
        out += '\n'
    string_write(logfile, out)
    if VERBOSE:
        sys.stderr.write(out)

def log(*args):
    out = "%s: %s:" % (time.asctime(), os.path.basename(sys.argv[0]))
    base_log(out, *args)

def _copy_files(dirobj, spec, fn):
    split = [x.split(':') for x in spec]
    for f, d in split:
        if d not in dirobj.dirs:
            raise RuntimeError("Unknown destination directory")
        fn(f, dirobj.dirname[d])

def copy_files(dirobj, spec):
    return _copy_files(dirobj, spec, shutil.copy)

def move_files(dirobj, spec):
    return _copy_files(dirobj, spec, shutil.move)

def main():
    global VERBOSE
    global logfile
    parser = optparse.OptionParser()
    parser.add_option("--copy", action="append", type="string", dest="copy",
                      default=[])
    parser.add_option("--move", action="append", type="string", dest="move",
                      default=[])
    parser.add_option("--basedir", action="store", type="string",
                      dest="basedir")
    parser.add_option("--daemon-timeout", action="store", type="int",
                      dest="daemon_timeout", default=60)
    parser.add_option("--verbose", action="store_true", dest="verbose",
                      default=False)
    parser.add_option("--overwrite-dirs", action="store_true", dest="overwrite",
                      default=False)
    parser.add_option("--log-level", action="store", type="string",
                      dest="log_level", default="info")
    (options, args) = parser.parse_args()
    VERBOSE = options.verbose

    # Create the dirs
    dirobj = Dirobject(overwrite=options.overwrite, basedir=options.basedir)
    dirobj.dirs = ['incoming', 'archive', 'error', 'root', 'log']
    dirobj.create_dirs()

    # Make the log file
    logfile = open(dirobj.get_path('log', 'rwflowappend-daemon.log'), 'wb', 0)

    # Generate the subprocess arguments
    args += ['--log-dest', 'stderr',
             '--log-level', options.log_level,
             '--no-daemon',
             '--root-directory', dirobj.dirname['root'],
             '--incoming-directory', dirobj.dirname['incoming'],
             '--archive-directory', dirobj.dirname['archive'],
             '--error-directory', dirobj.dirname['error']]

    progname = os.environ.get("RWFLOWAPPEND",
                              os.path.join('.', 'rwflowappend'))
    args = progname.split() + args

    # Set up state variables
    count = 0
    clean = False
    term = False
    send_list = None
    limit = len(options.copy) + len(options.move)
    regexp = re.compile("APPEND OK")
    closing = re.compile("Stopped logging")

    # Copy or move data
    copy_files(dirobj, options.copy)
    move_files(dirobj, options.move)

    # Note the time
    starttime = time.time()
    shutdowntime = None

    # Start the process
    log("Running", "'%s'" % "' '".join(args))
    proc = subprocess.Popen(args, stderr=subprocess.PIPE)
    line_reader = TimedReadline(proc.stderr.fileno())

    try:
        # Read the output data
        line = line_reader(1)
        while line is not None:
            if line:
                base_log(line)

            # Check for timeout
            if time.time() - starttime > options.daemon_timeout and not term:
                shutdowntime = time.time()
                log("Timed out")
                log("Sending SIGTERM")
                term = True
                try:
                    os.kill(proc.pid, signal.SIGTERM)
                except OSError:
                    pass

            # Match record counts
            match = regexp.search(line)
            if match:
                count += 1
                # Reset the timer if we are still receiving data
                starttime = time.time()
                if count >= limit and not term:
                    shutdowntime = time.time()
                    log("Reached limit")
                    log("Sending SIGTERM")
                    term = True
                    try:
                        os.kill(proc.pid, signal.SIGTERM)
                    except OSError:
                        pass

            # Check for clean shutdown
            match = closing.search(line)
            if match:
                clean = True

            # Check for timeout on SIGTERM shutdown
            if shutdowntime and time.time() - shutdowntime > 15:
                log("Timeout on SIGTERM.  Sending SIGKILL")
                shutdowntime = None
                try:
                    os.kill(proc.pid, signal.SIGKILL)
                except OSError:
                    pass

            # Get next line
            line = line_reader(1)

    finally:
        # Stop sending network data
        if send_list:
            stop_network_data(send_list)

        # Sleep briefly before polling for exit.
        time.sleep(1)
        proc.poll()
        if proc.returncode is None:
            log("Daemon has not exited.  Sending SIGKILL")
            try:
                os.kill(proc.pid, signal.SIGKILL)
            except OSError:
                pass

    # Print record count
    print("File count:", count)
    base_log("File count:", count)
    if limit != count:
        print(("ERROR: expecting %s files, got %s files" %
               (limit, count)))
        base_log(("ERROR: expecting %s files, got %s files" %
                  (limit, count)))

    # Check for clean exit
    if not clean:
        print("ERROR: shutdown was not clean")
        base_log("ERROR: shutdown was not clean")

    # Exit with the process's error code
    if proc.returncode is None:
        log("Exiting: Deamon did not appear to terminate normally")
        sys.exit(1)
    else:
        log("Exiting: Daemon returned", proc.returncode)
        sys.exit(proc.returncode)


if __name__ == "__main__":
    try:
        main()
    except SystemExit:
        raise
    except:
        traceback.print_exc(file=sys.stdout)
        if logfile:
            traceback.print_exc(file=logfile)
