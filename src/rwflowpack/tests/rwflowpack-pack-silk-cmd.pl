#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: rwflowpack-pack-silk-cmd.pl 40a363507ed0 2014-04-01 14:09:52Z mthomas $")

use strict;
use SiLKTests;
use File::Find;

my $rwflowpack = check_silk_app('rwflowpack');

# find the apps we need.  this will exit 77 if they're not available
my $rwfilter = check_silk_app('rwfilter');
my $rwuniq = check_silk_app('rwuniq');

# find the data files we use as sources, or exit 77
my %file;
$file{data} = get_data_or_exit77('data');

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

# the directory to hold the result of running the commands
my $cmd_dir = "$tmpdir/cmdout";
mkdir $cmd_dir
    or skip_test("Cannot create cmdout directory: $!");

# Generate the sensor.conf file
my $sensor_conf = "$tmpdir/sensor-templ.conf";
make_packer_sensor_conf($sensor_conf, 'silk', 0, 'polldir');

# create the input files
my @input_files = (
    File::Temp::mktemp("$tmpdir/file0.XXXXXX"),
    File::Temp::mktemp("$tmpdir/file1.XXXXXX"),
    File::Temp::mktemp("$tmpdir/file2.XXXXXX"),
    );

my $cmd = ("$rwfilter --type=in --sensor=S8 --pass=-"
           ." --stime=2009/02/12:01-2009/02/12:01 $file{data}"
           ." | $rwfilter --input-pipe=- --proto=6 --pass=$input_files[0]"
           ." --fail=- | $rwfilter --input-pipe=- --proto=17 --print-volume"
           ." --pass=$input_files[1] --fail=$input_files[2] 2>&1");
check_md5_output('2e4cc5e48c26a9d44c71fd7e977a9258', $cmd);

# the command that wraps rwflowpack
$cmd = join " ", ("$SiLKTests::PYTHON $srcdir/tests/rwflowpack-daemon.py",
                  ($ENV{SK_TESTS_VERBOSE} ? "--verbose" : ()),
                  ($ENV{SK_TESTS_LOG_DEBUG} ? "--log-level=debug" : ()),
                  "--sensor-conf=$sensor_conf",
                  "--output-mode=incremental-files",
                  "--copy $input_files[0]:incoming",
                  "--copy $input_files[1]:incoming",
                  "--copy $input_files[2]:incoming",
                  "--limit=298",
                  "--basedir=$tmpdir",
                  "--flush-timeout=5",
                  "--",
                  "--polling-interval=5",
                  "--post-archive-command='cp %s $cmd_dir/.'",
    );

# run it and check the MD5 hash of its output
check_md5_output('f67164d8e418abe9ca7c495e078cbb26', $cmd);


# the following directories should be empty
verify_empty_dirs($tmpdir, qw(error incoming sender root));

# verify files are in the archive directory and the cmd_dir
verify_archived_files("$tmpdir/archive", @input_files);

# verify files are in the $cmd_dir
verify_archived_files($cmd_dir, @input_files);

# path to the sender directory
my $data_dir = "$tmpdir/incremental";
die "ERROR: Missing data directory '$data_dir'\n"
    unless -d $data_dir;

# combine the files in sender dir and check the output
$cmd = ("$rwuniq --fields=1-5,type --ipv6-policy=ignore --sort-output"
        ." --values=records,packets,sTime-Earliest,eTime-Latest"
        ." ".join " ", glob("$data_dir/?*"));
check_md5_output('3363f7eb63a3ddb67ceae01088216763', $cmd);

exit 0;
