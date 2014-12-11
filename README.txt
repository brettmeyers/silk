SiLK, the System for Internet-Level Knowledge, is a collection of
traffic analysis tools developed by the CERT Network Situational
Awareness Team (CERT NetSA) to facilitate security analysis of large
networks. The SiLK tool suite supports the efficient collection,
storage, and analysis of network flow data, enabling network security
analysts to rapidly query large historical traffic data sets. SiLK is
ideally suited for analyzing traffic on the backbone or border of a
large, distributed enterprise or mid-sized ISP.

SiLK comes with NO WARRANTY.  The SiLK software components are
released under the GNU Public License V2 and Government Purpose
License Rights.  See the files doc/LICENSES.txt and doc/GPL.txt for
details.  Some included library code is covered under LGPL 2.1; see
source files for details.

In general, you can install SiLK by running
    configure ; make ; make install
However, there are several different configuration options.
Descriptions of the configation options and detailed installation
instructions are available in the SiLK Installation Handbook,
available in doc/silk-install-handbook.pdf and at
http://tools.netsa.cert.org/silk/silk-install-handbook.html or
http://tools.netsa.cert.org/silk/silk-install-handbook.pdf

Manual pages for each tool are installed when SiLK is installed.
These manual pages and additional documentation are available from
http://tools.netsa.cert.org/silk/

The following documents are available in the doc/ directory:
    RELEASE-NOTES.txt
        -- history of changes to SiLK
    LICENSES.txt
        -- brief description of licenses under which SiLK is released
    GPL.txt
        -- complete text of the GNU Public License V2
    silk-install-handbook.pdf
        -- complete description of installation, including various
           configurations you may choose
    silk-acceptance-tests.pdf
        -- description of tests used to check the integrity of SiLK
    autotools-readme.txt
        -- information for developers on using the AutoTools suite to
           re-create the "configure" script and "Makefile.in" files
