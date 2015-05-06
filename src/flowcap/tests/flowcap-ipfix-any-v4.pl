#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: flowcap-ipfix-any-v4.pl 2b1355d69f79 2015-02-13 23:12:41Z mthomas $")

use strict;
use SiLKTests;

my $NAME = $0;
$NAME =~ s,.*/,,;

# find the apps we need.  this will exit 77 if they're not available
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my $rwsilk2ipfix = check_silk_app('rwsilk2ipfix');

# find the data files we use as sources, or exit 77
my %file;
$file{data} = get_data_or_exit77('data');

# verify that required features are available
check_features(qw(ipfix));

# prefix any existing PYTHONPATH with the proper directories
check_python_bin();

# create our tempdir
my $tmpdir = make_tempdir();

# send data to this port and host
my $host = '127.0.0.1';
my $port = get_ephemeral_port($host, 'tcp');

# create the sensor.conf
my $sensor_conf = "$tmpdir/sensor.conf";
my $sensor_conf_text = <<EOF;
probe P0 ipfix
    protocol tcp
    listen-on-port $port
    accept-from-host 127.127.127.127 $host
end probe
EOF
make_config_file($sensor_conf, \$sensor_conf_text);

# Generate the test data
my $ipfixdata = "$tmpdir/data.ipfix";
unlink $ipfixdata;
system "$rwsilk2ipfix --ipfix-output=$ipfixdata $file{data}"
    and die "$NAME: ERROR: Failed running rwsilk2ipfix\n";

# the command that wraps flowcap
my $cmd = join " ", ("$SiLKTests::PYTHON $srcdir/tests/flowcap-daemon.py",
                     ($ENV{SK_TESTS_VERBOSE} ? "--verbose" : ()),
                     ($ENV{SK_TESTS_LOG_DEBUG} ? "--log-level=debug" : ()),
                     "--tcp $ipfixdata,$host,$port",
                     "--limit=501876",
                     "--basedir=$tmpdir",
                     "--",
                     "--sensor-conf=$sensor_conf",
                     "--max-file-size=100k",
    );

# run it and check the MD5 hash of its output
check_md5_output('a78a286719574389a972724d761c931e', $cmd);

# path to the directory holding the output files
my $data_dir = "$tmpdir/destination";
die "$NAME: ERROR: Missing data directory '$data_dir'\n"
    unless -d $data_dir;

# check for zero length files in the directory
opendir D, "$data_dir"
    or die "$NAME: ERROR: Unable to open directory $data_dir: $!\n";
for my $f (readdir D) {
    next if (-d "$data_dir/$f") || (0 < -s _);
    warn "$NAME: WARNING: Zero length files in $data_dir\n";
    last;
}
closedir D;

# create a command to sort all files in the directory and output them
# in a standard form.
$cmd = ("find $data_dir -type f -print "
        ." | $rwcat --xargs "
        ." | $rwsort --fields=stime,sip "
        ." | $rwcat --byte-order=little --compression-method=none"
        ." --ipv4-output");

# because the IPv4 flowcap file format encodes bytes as a
# bytes-per-packet and the IPv6 does not, the results will be slightly
# different depending on whether SiLK was compiled with IPv6 support
my $md5 = (($SiLKTests::SK_ENABLE_IPV6)
           ? '796448848fa25365cd3500772b9a9649'
           : 'fdf2663a584e900d17aaa577d47bdd0f');

exit check_md5_output($md5, $cmd);
