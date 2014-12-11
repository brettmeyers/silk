

To build the 'configure' script and 'Makefile.in' files that configure
turns into 'Makefile's, you should run

  autoreconf -f -i -v

as that will normally work for the SiLK source code.



However, if

  autoreconf -f -i -v

exits with an error message, you will need to run the following in
this order:

 0. autoconf requires the GNU version of m4; either have it first on
    your path, or set the M4 environment variable to its location

 1. aclocal -I m4

        If your 'aclocal' is not aware of the M4 macros that are
        provided by 'pkg-config' (specifically PKG_CHECK_MODULES), you
        should include a -I argument to the directory containing the
        pkg.m4 file that defines PKG_CHECK_MODULES.  For example, on
        the Mac, I run:

        aclocal -I m4 -I /opt/local/share/aclocal

        If you fail to provide the location of 'pkg.m4', aclocal will
        silently succeed and the generated 'configure' script will not
        support the use of GnuTLS, IPFIX, and IPA.

 2. autoheader

 3. libtoolize --force --copy

        Note that this is called 'glibtoolize' on the Mac

 4. automake --add-missing --copy

 5. autoconf

 6. Now, at last, you can run ./configure


# $SiLK: autotools-readme.txt a302b0b9b735 2011-04-11 21:16:02Z mthomas $
