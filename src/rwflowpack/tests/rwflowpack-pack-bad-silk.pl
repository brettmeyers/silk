#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: rwflowpack-pack-bad-silk.pl 40a363507ed0 2014-04-01 14:09:52Z mthomas $")

use strict;
use SiLKTests;
use File::Temp ();

my $rwflowpack = check_silk_app('rwflowpack');

# find the apps we need.  this will exit 77 if they're not available

# find the data files we use as sources, or exit 77
my %file;
$file{empty} = get_data_or_exit77('empty');

# prefix any existing PYTHONPATH with the proper directories
check_python_bin();

# set the environment variables required for rwflowpack to find its
# packing logic plug-in
add_plugin_dirs('/site/twoway');

# Skip this test if we cannot load the packing logic
check_exit_status("$rwflowpack --sensor-conf=$srcdir/tests/sensor77.conf"
                  ." --verify-sensor-conf")
    or skip_test("Cannot load packing logic");

# create our tempdir
my $tmpdir = make_tempdir();

# Generate the sensor.conf file
my $sensor_conf = "$tmpdir/sensor-templ.conf";
make_packer_sensor_conf($sensor_conf, 'silk', 0, 'polldir');

# the invalid files
my %inval;

# file handle
my $fh;

# create a completely invalid file containing the file's own name
($fh, $inval{junk}) = File::Temp::tempfile("$tmpdir/invalid.XXXXXX");
print $fh $inval{junk};
close $fh
    or die "ERROR: Cannot close $inval{junk}: $!\n";

# create a copy of the empty SiLK data file
$inval{empty} = File::Temp::mktemp("$tmpdir/empty.XXXXXX");
system "cp", $file{empty}, $inval{empty};

# the command that wraps rwflowpack
my $cmd = join " ", ("$SiLKTests::PYTHON $srcdir/tests/rwflowpack-daemon.py",
                     ($ENV{SK_TESTS_VERBOSE} ? "--verbose" : ()),
                     ($ENV{SK_TESTS_LOG_DEBUG} ? "--log-level=debug" : ()),
                     "--sensor-conf=$sensor_conf",
                     (map {"--move $inval{$_}:incoming"} keys %inval),
                     "--basedir=$tmpdir",
                     "--daemon-timeout=20",
                     "--",
                     "--polling-interval=5",
    );

# run it and check the MD5 hash of its output
check_md5_output('8d06e798951bc231967e43b2f18f3499', $cmd);

# the following directories should be empty
verify_empty_dirs($tmpdir, qw(incoming incremental root sender));

# verify files are in the archive directory
verify_archived_files("$tmpdir/archive", $inval{empty});

# verify files are in the error directory
verify_directory_files("$tmpdir/error", $inval{junk});

exit 0;
