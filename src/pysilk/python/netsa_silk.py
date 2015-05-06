# Copyright 2008-2015 by Carnegie Mellon University

# @OPENSOURCE_HEADER_START@
# Use of the netsa_silk support library and related source code is
# subject to the terms of the following licenses:
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
# @OPENSOURCE_HEADER_END@

"""
The netsa_silk module contains a shared API for working with common
Internet data in both netsa-python and PySiLK.  If netsa-python is
installed but PySiLK is not, the less efficient but more portable
pure-Python version of this functionality that is included in
netsa-python is used.  If PySiLK is installed, then the
high-performance C version of this functionality that is included in
PySiLK is used.
"""

# This module exists only to import the needed functionality from
# either PySiLK (via the internal silk._netsa_silk module), or
# netsa-python (via the internal netsa._netsa_silk module) and then
# re-export it.  All information (including the list of symbols to
# export) comes from the module providing the functionality, so that
# this module will never have to change and may remain common to both
# PySiLK and netsa-python installs.  (That is: Since it's always the
# same, it's acceptable for both sources to install the file without
# fear of overwriting each other.

import os

try:
    if os.getenv("NETSA_SILK_DISABLE_PYSILK"):
        raise ImportError("netsa_silk PySiLK support disabled")
    import silk._netsa_silk
    from silk._netsa_silk import *
    __all__ = silk._netsa_silk.__all__
except ImportError:
    try:
        if os.getenv("NETSA_SILK_DISABLE_NETSA_PYTHON"):
            raise ImportError("netsa_silk netsa-python support disabled")
        import netsa._netsa_silk
        from netsa._netsa_silk import *
        __all__ = netsa._netsa_silk.__all__
    except ImportError:
        import_error = ImportError(
            "Cannot locate netsa_silk implementation during import")
        raise import_error
