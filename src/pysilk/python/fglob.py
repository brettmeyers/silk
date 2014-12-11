#######################################################################
# Copyright (C) 2008-2014 by Carnegie Mellon University.
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
# $SiLK: fglob.py 1c87fe1a22f7 2014-01-06 15:24:30Z mthomas $
#######################################################################

from subprocess import Popen, PIPE
from datetime import date, datetime, time
import errno
import sys

# Python 3.0 doesn't have basestring
if sys.hexversion >= 0x03000000:
    basestring = str

__all__ = ['FGlob']

rwfglob_executable = "rwfglob"

class FGlob(object):

    def __init__(self,
                 classname=None,
                 type=None,
                 sensors=None,
                 start_date=None,
                 end_date=None,
                 data_rootdir=None,
                 site_config_file=None):

        global rwfglob_executable

        if not (classname or type or sensors or start_date or end_date):
            raise ValueError("Must contain a specification")

        if end_date and not start_date:
            raise ValueError("An end_date requires a start_date")

        if isinstance(sensors, list):
            sensors = ",".join(map(str, sensors))
        elif isinstance(sensors, basestring):
            sensors = str(sensors)

        if isinstance(type, list):
            type = ",".join(type)
        elif isinstance(type, basestring):
            type = str(type)

        if isinstance(start_date, datetime):
            start_date = start_date.strftime("%Y/%m/%d:%H")
        elif isinstance(start_date, date):
            start_date = start_date.strftime("%Y/%m/%d")
        elif isinstance(start_date, time):
            start_date = datetime.combine(date.today(), start_date)
            start_date = start_date.strftime("%Y/%m/%d:%H")

        if isinstance(end_date, datetime):
            end_date = end_date.strftime("%Y/%m/%d:%H")
        elif isinstance(end_date, date):
            end_date = end_date.strftime("%Y/%m/%d")
        elif isinstance(end_date, time):
            end_date = datetime.combine(date.today(), end_date)
            end_date = end_date.strftime("%Y/%m/%d:%H")

        self.args = [rwfglob_executable, "--no-summary"]
        for val, arg in [(classname, "class"),
                         (type, "type"),
                         (sensors, "sensors"),
                         (start_date, "start-date"),
                         (end_date, "end-date"),
                         (data_rootdir, "data-rootdir"),
                         (site_config_file, "site-config-file")]:
            if val:
                if not isinstance(val, str):
                    raise ValueError("Specifications must be strings")
                self.args.append("--%s" % arg)
                self.args.append(val)

    def __iter__(self):
        try:
            rwfglob = Popen(self.args, bufsize=1, stdout=PIPE, close_fds=True,
                            universal_newlines=True)
        except OSError:
            # Handled using sys.exc_info() in order to compile on 2.4
            # and 3.0
            e = sys.exc_info()[1]
            if e.errno != errno.ENOENT:
                raise
            raise RuntimeError("Cannot find the %s program in PATH" %
                               self.args[0])
        for line in rwfglob.stdout:
            if rwfglob.returncode not in [None, 0]:
                raise RuntimeError("rwfglob failed")
            yield line.rstrip('\n\r')
