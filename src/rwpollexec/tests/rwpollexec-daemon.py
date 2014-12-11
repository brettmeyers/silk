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
# $SiLK: rwpollexec-daemon.py 39e8161c33bb 2014-03-31 18:57:22Z mthomas $
#######################################################################
from __future__ import print_function
import optparse
import subprocess
import signal
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
    if logfile:
        string_write(logfile, out)
    if VERBOSE:
        sys.stderr.write(out)

def log(*args):
    out = "%s: %s:" % (time.asctime(), os.path.basename(sys.argv[0]))
    base_log(out, *args)

exit_sig_value = 255

def exit_sig(sig, frame):
    sys.exit(exit_sig_value)

def execute(args):
    global exit_sig_value
    if len(args) != 1:
        log("Requires a single file option in execute mode")
        sys.exit(255)
    lines = open(args[0], 'r').readlines()
    command = lines[0].strip()
    if command == "hang":
        for i in range(0, signal.NSIG):
            try:
                signal.signal(i, signal.SIG_IGN)
            except ValueError:
                pass
            except RuntimeError:
                pass
            except OSError:
                # In Python 3.3, signal.signal() raises OSError
                # instead of RuntimeError
                pass
        while True:
            signal.pause()
    elif command == "term":
        val = int(lines[1])
        exit_sig_value = val
        signal.signal(signal.SIGTERM, exit_sig)
        while True:
            signal.pause()
    elif command == "return":
        val = int(lines[1])
        sys.exit(val)
    elif command == "signal":
        val = int(lines[1])
        os.kill(os.getpid(), val)
    else:
        sys.exit(255)

def handle_file_op(option, opt_str, value, parser, *args, **kwds):
    if value == None:
        value = "0"
    else:
        value = str(value)
    if opt_str == "--hang":
        parser.values.infile.append(("hang",""))
    elif opt_str == "--term":
        parser.values.infile.append(("term", value))
    elif opt_str == "--return":
        parser.values.infile.append(("return", value))
    elif opt_str == "--signal":
        parser.values.infile.append(("signal", value))
    else:
        sys.exit(255)

def create_files(dirobj, spec):
    i = 0
    for (item, value) in spec:
        name = "%0d" % (i,)
        i += 1
        loc = os.path.join(dirobj.dirname['incoming'], name)
        f = open(loc, 'w')
        f.writelines((item, '\n', value, '\n'))
        f.close()

def main():
    global VERBOSE
    global logfile
    parser = optparse.OptionParser()
    parser.set_defaults(infile=[])
    parser.add_option("--hang", action="callback", callback=handle_file_op)
    parser.add_option("--term", action="callback", type="int",
                      callback=handle_file_op)
    parser.add_option("--return", action="callback", type="int",
                      callback=handle_file_op)
    parser.add_option("--signal", action="callback", type="int",
                      callback=handle_file_op)
    parser.add_option("--exec", action="store_true", dest="execute",
                      default=False)
    parser.add_option("--basedir", action="store", type="string",
                      dest="basedir")
    parser.add_option("--no-archive", action="store_true", dest="no_archive",
                      default=False)
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

    if options.execute:
        execute(args)           # Does not return


    # Create the dirs
    dirobj = Dirobject(overwrite=options.overwrite, basedir=options.basedir)
    dirobj.dirs = ['incoming', 'archive', 'error', 'log']
    dirobj.create_dirs()

    # Make the log file
    logfile = open(dirobj.get_path('log', 'rwpollexec-daemon.log'), 'wb', 0)

    # Generate the subprocess arguments
    args += ['--log-dest', 'stderr',
             '--log-level', options.log_level,
             '--no-daemon',
             '--incoming-directory', dirobj.dirname['incoming'],
             '--error-directory', dirobj.dirname['error']]
    if not options.no_archive:
        args += ['--archive-directory', dirobj.dirname['archive']]

    progname = os.environ.get("RWPOLLEXEC",
                              os.path.join('.', 'rwpollexec'))
    args = progname.split() + args

    # Set up state variables
    run_count = 0
    complete_count = 0
    success_count = 0
    nonzero_count = 0
    killed_count = 0
    clean = False
    term = False
    limit = len(options.infile)
    regexp = re.compile("Running \[\d+\]:")
    success = "Command .* has completed successfully"
    nonzero = "Command .* has completed with a nonzero return"
    killed = "Command .* was terminated by SIG"
    complete = re.compile("(?P<success>%s)|(?P<nonzero>%s)|(?P<killed>%s)" %
                          (success, nonzero, killed))
    closing = re.compile("Stopped logging")

    if limit == 0:
        parser.print_usage()
        sys.exit(255)

    # Create data
    create_files(dirobj, options.infile)

    # Note the time
    starttime = time.time()
    shutdowntime = None
    endedtime = None

    # Start the process
    log("Running", "'%s'" % "' '".join(args))
    proc = subprocess.Popen(args, stderr=subprocess.PIPE,
                            preexec_fn=lambda:os.setpgid(0, 0))
    try:
        os.setpgid(proc.pid, 0)
    except OSError:
        pass
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

            # Match running counts
            match = regexp.search(line)
            if match:
                run_count += 1
                # Reset the timer if we are still receiving data
                starttime = time.time()

            # Match success counts
            match = complete.search(line)
            if match:
                if match.group('success') is not None:
                    success_count += 1
                elif match.group('nonzero') is not None:
                    nonzero_count += 1
                elif match.group('killed') is not None:
                    killed_count += 1
                complete_count += 1
                # Reset the timer if we are still receiving data
                starttime = time.time()
                if complete_count >= limit and not term:
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
                    os.killpg(proc.pid, signal.SIGKILL)
                    break
                except OSError:
                    pass

            # Check to see if the process has ended
            if endedtime is None:
                proc.poll()
                if proc.returncode is not None:
                    endedtime = time.time()

            # Allow some time after the process has ended for any
            # remaining output the be handled
            if endedtime and time.time() - endedtime > 5:
                break

            # Get next line
            line = line_reader(1)

    finally:
        # Sleep briefly before polling for exit.
        time.sleep(1)
        proc.poll()
        if proc.returncode is None:
            log("Daemon has not exited.  Sending SIGKILL")
            try:
                os.killpg(proc.pid, signal.SIGKILL)
            except OSError:
                pass

    # Kill everything for good luck
    try:
        os.killpg(proc.pid, signal.SIGKILL)
    except OSError:
        pass

    # Print record count
    print("Run count:", run_count)
    print(("Complete count (success, failure): (%s, %s) = %s" %
           (success_count, nonzero_count + killed_count, complete_count)))
    base_log("Run count:", run_count)
    base_log(("Complete count (success, failure): (%s, %s) = %s" %
              (success_count, nonzero_count + killed_count, complete_count)))
    if limit != complete_count:
        print(("ERROR: expecting %s files, got %s files" %
               (limit, complete_count)))
        base_log(("ERROR: expecting %s files, got %s files" %
                  (limit, complete_count)))

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
