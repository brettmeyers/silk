#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: rwflowappend-append-bad.pl 0942a344ff30 2014-04-01 19:46:23Z mthomas $")

use strict;
use SiLKTests;
use File::Temp ();


# find the apps we need.  this will exit 77 if they're not available
my $rwcat = check_silk_app('rwcat');
my $rwfilter = check_silk_app('rwfilter');

# find the data files we use as sources, or exit 77
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');

# prefix any existing PYTHONPATH with the proper directories
check_python_bin();

# create our tempdir
my $tmpdir = make_tempdir();

# create the files
my %input_files = (
    badname   => File::Temp::mktemp("$tmpdir/bad-name.XXXXXX"),
    nonsilk   => File::Temp::mktemp("$tmpdir/in-S8_20090212.01.XXXXXX"),
    empty     => File::Temp::mktemp("$tmpdir/in-S8_20090212.01.XXXXXX"),
    truncate1 => File::Temp::mktemp("$tmpdir/in-S8_20090212.02.XXXXXX"),
    truncate2 => File::Temp::mktemp("$tmpdir/in-S8_20090212.02.XXXXXX"),
    );

# the empty file contains no records
system("cp", $file{empty}, $input_files{empty})
    and die "ERROR: Cannot copy file to '$input_files{empty}'\n";

# badname is a valid SiLK file but does not have the proper header or
# proper name
link $input_files{empty}, $input_files{badname}
    or die "ERROR: Cannot create link '$input_files{badname}': $!\n";

# create a completely invalid SiLK flow file by having a file that
# contains the file's name as text
open F, ">$input_files{nonsilk}"
    or die "ERROR: Unable to open '$input_files{nonsilk}: $!\n";
print F $input_files{nonsilk};
close F;

# create a file with two valid records, then shave a few bytes off the
# second record to test a short-read; use this file twice
my $cmd = ("$rwfilter --type=in --sensor=S8 --pass=$input_files{truncate1}"
           ." --stime=2009/02/12:02-2009/02/12:02 --max-pass=2"
           ." --print-volume --compression-method=none $file{data} 2>&1");
check_md5_output('8132a5d39d1b4caed01861fe029c703e', $cmd);

open F, "+<", $input_files{truncate1}
    or die "ERROR: Cannot open '$input_files{truncate1}' for update: $!'\n";
binmode F;
truncate F, ((-s F) - 3)
    or die "ERROR: Cannot truncate '$input_files{truncate1}': $!\n";
close F
    or die "ERROR: Cannot close '$input_files{truncate1}': $!\n";

link $input_files{truncate1}, $input_files{truncate2}
    or die "ERROR: Cannot create link '$input_files{truncate2}': $!\n";


# the command that wraps rwflowpack
$cmd = join " ", ("$SiLKTests::PYTHON $srcdir/tests/rwflowappend-daemon.py",
                  ($ENV{SK_TESTS_VERBOSE} ? "--verbose" : ()),
                  ($ENV{SK_TESTS_LOG_DEBUG} ? "--log-level=debug" : ()),
                  (map {"--copy $input_files{$_}:incoming"} keys %input_files),
                  "--basedir=$tmpdir",
                  "--daemon-timeout=15",
                  "--",
                  "--polling-interval=5",
                  "--threads=4",
    );

# run it and check the MD5 hash of its output
check_md5_output('959cfff5b697a519cdefaf21b16e328e', $cmd);


# the following directories should be empty
verify_empty_dirs($tmpdir, qw(incoming));

# verify files are in the archive directory
verify_directory_files(
    "$tmpdir/archive/in/2009/02/12",
    map {$input_files{$_}} grep {!/(nonsilk|badname)/} keys %input_files);

# verify files are in the error directory
verify_directory_files(
    "$tmpdir/error",
    map {$input_files{$_}} grep {/(nonsilk|badname)/} keys %input_files);

# expected data file
my $data_file = "$tmpdir/root/in/2009/02/12/in-S8_20090212.02";
die "ERROR: Missing data file '$data_file'\n"
    unless -f $data_file;

# compute MD5 of data file
$cmd = "$rwcat --compression=none --byte-order=little --ipv4-output $data_file";
check_md5_output("16dfcd096f21cebb0927ffe4b86fc8ca", $cmd);

exit 0;
