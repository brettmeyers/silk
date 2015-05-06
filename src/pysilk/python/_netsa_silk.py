#######################################################################
# Copyright (C) 2011-2015 by Carnegie Mellon University.
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
# $SiLK: _netsa_silk.py b7b8edebba12 2015-01-05 18:05:21Z mthomas $
#######################################################################

"""
The netsa_silk module contains a shared API for working with common
Internet data in both netsa-python and PySiLK.  If netsa-python is
installed but PySiLK is not, the less efficient but more portable
pure-Python version of this functionality that is included in
netsa-python is used.  If PySiLK is installed, then the
high-performance C version of this functionality that is included in
PySiLK is used.
"""

# This module provides the symbols exported by PySiLK for the
# netsa_silk API.  It exists to rename PySiLK symbols that have a
# different name from the netsa_silk symbols, and to constrain the set
# of PySiLK symbols that are exported.  If a new symbol is added (to
# provide a new feature), it need only be added here and it will
# automatically be exported by netsa_silk.

from silk import (
    ipv6_enabled as has_IPv6Addr,
    IPAddr, IPv4Addr, IPv6Addr,
    IPSet as ip_set,
    IPWildcard,
    TCPFlags,

    TCP_FIN, TCP_SYN, TCP_RST, TCP_PSH, TCP_ACK, TCP_URG, TCP_ECE, TCP_CWR,
    silk_version
)

# PySiLK API version
__version__ = "1.0"

# Implementation version
__impl_version__ = " ".join(["SiLK", silk_version()])

__all__ = """
    has_IPv6Addr
    IPAddr IPv4Addr IPv6Addr
    ip_set
    IPWildcard
    TCPFlags

    TCP_FIN TCP_SYN TCP_RST TCP_PSH TCP_ACK TCP_URG TCP_ECE TCP_CWR

    __version__
    __impl_version__
""".split()
