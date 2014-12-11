#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: rwpollexec-set-noarchive.pl 40a363507ed0 2014-04-01 14:09:52Z mthomas $")

use strict;
use SiLKTests;
use File::Temp ();


# prefix any existing PYTHONPATH with the proper directories
check_python_bin();

# create our tempdir
my $tmpdir = make_tempdir();

# the command that wraps rwpollexec
my $rwpollexec_py = "$SiLKTests::PYTHON $srcdir/tests/rwpollexec-daemon.py";
my $cmd = join " ", ("$rwpollexec_py",
                     ($ENV{SK_TESTS_VERBOSE} ? "--verbose" : ()),
                     ($ENV{SK_TESTS_LOG_DEBUG} ? "--log-level=debug" : ()),
                     "--return 0",
                     "--return 1",
                     "--signal 9",
                     "--term 0",
                     "--term 1",
                     "--hang",
                     "--return 0",
                     "--return 250",
                     "--signal 3",
                     "--term 0",
                     "--term 127",
                     "--hang",
                     "--basedir=$tmpdir",
                     "--no-archive",
                     "--",
                     "--flat-archive",
                     "--polling-interval=3",
                     "--command \"$rwpollexec_py --exec %s\"",
                     "--timeout TERM,3",
                     "--timeout KILL,5"
    );

my @expected_archive = qw();
my @expected_error   = qw(1 2 4 5 7 8 10 11);

# run it and check the MD5 hash of its output
check_md5_output('ba7517a640382f4281b605f572bdaaa6', $cmd);


# the following directories should be empty
verify_empty_dirs($tmpdir, qw(incoming));

# verify files are in the archive directory
verify_directory_files("$tmpdir/archive", @expected_archive);

# verify files are in the error directory
verify_directory_files("$tmpdir/error", @expected_error);

