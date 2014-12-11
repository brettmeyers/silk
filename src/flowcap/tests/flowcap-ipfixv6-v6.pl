#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: flowcap-ipfixv6-v6.pl 40a363507ed0 2014-04-01 14:09:52Z mthomas $")

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
$file{v6data} = get_data_or_exit77('v6data');

# verify that required features are available
check_features(qw(ipv6 inet6 ipfix));

# prefix any existing PYTHONPATH with the proper directories
check_python_bin();

# create our tempdir
my $tmpdir = make_tempdir();

# send data to this port and host
my $host = '::1';
my $port = get_ephemeral_port($host, 'tcp');

# create the sensor.conf
my $sensor_conf = "$tmpdir/sensor.conf";
my $sensor_conf_text = <<EOF;
probe P0 ipfix
    protocol tcp
    listen-on-port $port
    listen-as-host $host
end probe
EOF
make_config_file($sensor_conf, \$sensor_conf_text);

# Generate the test data
my $ipfixdata = "$tmpdir/data.ipfix";
unlink $ipfixdata;
system "$rwsilk2ipfix --ipfix-output=$ipfixdata $file{v6data}"
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
        ." | $rwcat --byte-order=little --compression-method=none");

my $md5 = 'f8b362a10ac1fcae3f85149dda6a2b4f';

exit check_md5_output($md5, $cmd);
