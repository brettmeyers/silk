#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: rwflowpack-pack-pdu-dir.pl 40a363507ed0 2014-04-01 14:09:52Z mthomas $")

use strict;
use SiLKTests;
use File::Find;

my $rwflowpack = check_silk_app('rwflowpack');

# find the apps we need.  this will exit 77 if they're not available
my $rwcat = check_silk_app('rwcat');

# find the data files we use as sources, or exit 77
my %file;
$file{pdu} = get_data_or_exit77('pdu_small');

# prefix any existing PYTHONPATH with the proper directories
check_python_bin();

# set the environment variables required for rwflowpack to find its
# packing logic plug-in
add_plugin_dirs('/site/twoway');

# Skip this test if we cannot load the packing logic
check_exit_status("$rwflowpack --sensor-conf=$srcdir/tests/sensor77.conf"
                  ." --verify-sensor-conf")
    or skip_test("Cannot load packing logic");

# Check whether the --packing-logic switch exists.  If not, the
# packing-logic was compiled into rwflowpack.
my @packing_logic_switch = qw(--packing-logic=packlogic-twoway.so);
unless (check_app_switch($rwflowpack, "packing-logic")) {
    @packing_logic_switch = ();
}

# create our tempdir
my $tmpdir = make_tempdir();

# Generate the sensor.conf file
my $sensor_conf = "$tmpdir/sensor-templ.conf";
make_packer_sensor_conf($sensor_conf, 'netflow-v5', 0, 'polldir');

# the command that wraps rwflowpack
my $cmd = join " ", ("$SiLKTests::PYTHON $srcdir/tests/rwflowpack-daemon.py",
                     ($ENV{SK_TESTS_VERBOSE} ? "--verbose" : ()),
                     ($ENV{SK_TESTS_LOG_DEBUG} ? "--log-level=debug" : ()),
                     "--sensor-conf=$sensor_conf",
                     "--copy $file{pdu}:incoming",
                     "--limit=50000",
                     "--basedir=$tmpdir",
                     "--",
                     @packing_logic_switch,
                     "--polling-interval=5",
    );

# run it and check the MD5 hash of its output
check_md5_output('dba69618fe40eafc6a1dca7d888db2b0', $cmd);


# the following directories should be empty
verify_empty_dirs($tmpdir, qw(error incoming incremental sender));

# path to the data directory
my $data_dir = "$tmpdir/root";
die "ERROR: Missing data directory '$data_dir'\n"
    unless -d $data_dir;

# number of files to find in the data directory
my $expected_count = 0;
my $file_count = 0;

# read in the MD5s for every packed file we expect to find.  Although
# we are packing IPv4 data, whether we write IPv4 or IPv6 files
# depends on how SiLK was compiled.  In the packed IPv4 files, bytes
# are stored as a byte/packet ratio, and due to rounding the "bytes"
# value in the IPv4 and IPv6 files may differ.  Thus, we read in
# separate MD5 sums for each.
my %md5_map;
my $md5_file = "$0-ipv".(($SiLKTests::SK_ENABLE_IPV6) ? "6" : "4").".txt";

open F, $md5_file
    or die "ERROR: Cannot open $md5_file: $!\n";
while (my $lines = <F>) {
    my ($md5, $path) = split " ", $lines;
    $md5_map{$path} = $md5;
    ++$expected_count;
}
close F;

# find the files in the data directory and compare their MD5 hashes
File::Find::find({wanted => \&check_file, no_chdir => 1}, $data_dir);

# did we find all our files?
if ($file_count != $expected_count) {
    die "ERROR: Found $file_count files in root; expected $expected_count\n";
}

# successful!
exit 0;


# this is called by File::Find::find.  The full path to the file is in
# the $_ variable
sub check_file
{
    # skip anything that is not a file
    return unless -f $_;
    my $path = $_;
    # set $_ to just be the file basename
    s,^.*/,,;
    die "ERROR: Unexpected file $path\n"
        unless $md5_map{$_};
    ++$file_count;

    # do the MD5 sums match?
    check_md5_output($md5_map{$_}, ("$rwcat --ipv4-output --byte-order=little"
                                    ." --compression-method=none $path"));
}


__DATA__
