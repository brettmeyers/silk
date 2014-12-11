#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: rwflowpack-pack-ipfix-net-v4.pl 40a363507ed0 2014-04-01 14:09:52Z mthomas $")

use strict;
use SiLKTests;
use File::Find;

my $rwflowpack = check_silk_app('rwflowpack');

# find the apps we need.  this will exit 77 if they're not available
my $rwcat = check_silk_app('rwcat');
my $rwsilk2ipfix = check_silk_app('rwsilk2ipfix');

# find the data files we use as sources, or exit 77
my %file;
$file{data} = get_data_or_exit77('data');

# verify that required features are available
check_features(qw(ipfix));

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

# send data to this port and host
my $host = '127.0.0.1';
my $port = get_ephemeral_port($host, 'tcp');

# Generate the sensor.conf file
my $sensor_conf = "$tmpdir/sensor-templ.conf";
make_packer_sensor_conf($sensor_conf, 'ipfix', $port, 'tcp4');

# Generate the test data
my $ipfixdata = "$tmpdir/data.ipfix";
unlink $ipfixdata;
system "$rwsilk2ipfix --ipfix-output=$ipfixdata $file{data}"
    and die "ERROR: Failed running rwsilk2ipfix\n";

# the command that wraps rwflowpack
my $cmd = join " ", ("$SiLKTests::PYTHON $srcdir/tests/rwflowpack-daemon.py",
                     ($ENV{SK_TESTS_VERBOSE} ? "--verbose" : ()),
                     ($ENV{SK_TESTS_LOG_DEBUG} ? "--log-level=debug" : ()),
                     "--sensor-conf=$sensor_conf",
                     "--tcp $ipfixdata,$host,$port",
                     "--limit=501876",
                     "--basedir=$tmpdir",
    );

# run it and check the MD5 hash of its output
check_md5_output('a78a286719574389a972724d761c931e', $cmd);


# the following directories should be empty
verify_empty_dirs($tmpdir, qw(archive error incoming incremental sender));

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
my $md5_file = $0;
# we can use the same MD5 sums as those for packing a SiLK file
$md5_file =~ s/-ipfix-net-v4/-silk/;

if ($SiLKTests::SK_ENABLE_IPV6) {
    $md5_file .= "-ipv6.txt";
}
else {
    $md5_file .= "-ipv4.txt";
}

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
                                    ." --compression=none $path"));
}


__DATA__
