#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: rwflowpack-pack-multiple2.pl 40a363507ed0 2014-04-01 14:09:52Z mthomas $")

use strict;
use SiLKTests;
use File::Find;

my $rwflowpack = check_silk_app('rwflowpack');

# find the apps we need.  this will exit 77 if they're not available
my $rwcat = check_silk_app('rwcat');
my $rwuniq = check_silk_app('rwuniq');

# find the data files we use as sources, or exit 77
my %file;
$file{data} = get_data_or_exit77('data');
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

# create our tempdir
my $tmpdir = make_tempdir();

# Generate the sensor.conf file
my $sensor_conf = "$tmpdir/sensor-templ.conf";
{
    # undef record separator to slurp all of <DATA> into variable
    local $/;
    my $sensor_conf_text = <DATA>;
    make_config_file($sensor_conf, \$sensor_conf_text);
}

# the command that wraps rwflowpack
my $cmd = join " ", ("$SiLKTests::PYTHON $srcdir/tests/rwflowpack-daemon.py",
                     ($ENV{SK_TESTS_VERBOSE} ? "--verbose" : ()),
                     ($ENV{SK_TESTS_LOG_DEBUG} ? "--log-level=debug" : ()),
                     "--sensor-conf=$sensor_conf",
                     "--copy $file{data}:incoming",
                     "--copy $file{data}:incoming2",
                     "--copy $file{pdu}:incoming3",
                     "--copy $file{pdu}:incoming4",
                     "--limit=1103752",
                     "--basedir=$tmpdir",
                     "--daemon-timeout=90",
                     "--",
                     "--polling-interval=5",
    );

# run it and check the MD5 hash of its output
check_md5_output('e4d9a9fe18a95da02c3cf1123e9b8139', $cmd);


# the following directories should be empty
verify_empty_dirs($tmpdir, qw(error incoming incremental sender));

# path to the data directory
my $data_dir = "$tmpdir/root";
die "ERROR: Missing data directory '$data_dir'\n"
    unless -d $data_dir;

# check the output
$cmd = ("find $data_dir -type f -print"
        ." | $rwcat --xargs"
        ." | $rwuniq --ipv6=ignore --fields=sip,sensor,type,stime"
        ." --values=records,packets,stime,etime --sort");
check_md5_output('5eda801132f3c80f58a417c7b973d3c7', $cmd);

# successful!
exit 0;

__DATA__
# sensor.conf file for this test
probe P0-silk silk
    poll-directory ${incoming}
end probe

probe P1 silk
    poll-directory ${incoming2}
end probe

probe P0-pdu netflow-v5
    poll-directory ${incoming3}
end probe

probe P2 netflow-v5
    poll-directory ${incoming4}
end probe

# sensor S0 is made up of two probes
sensor S0
    silk-probes P0-silk
    internal-ipblocks 192.168.x.x   #IPV6 , 2001:c0:a8::x:x
    external-ipblocks 10.0.0.0/8    #IPV6   2001:a:x::x:x
    null-ipblocks     172.16.0.0/13 #IPV6 , 2001:ac:10-17::x:x
end sensor

sensor S0
    netflow-v5-probes P0-pdu
    internal-ipblocks 192.168.x.x   #IPV6 , 2001:c0:a8::x:x
    external-ipblocks 10.0.0.0/8    #IPV6   2001:a:x::x:x
    null-ipblocks     172.16.0.0/13 #IPV6 , 2001:ac:10-17::x:x
end sensor

sensor S1
    silk-probes P1
    internal-ipblocks 192.168.x.x   #IPV6 , 2001:c0:a8::x:x
    external-ipblocks 10.0.0.0/8    #IPV6   2001:a:x::x:x
    null-ipblocks     172.16.0.0/13 #IPV6 , 2001:ac:10-17::x:x
end sensor

sensor S2
    netflow-v5-probes P2
    internal-ipblocks 192.168.x.x   #IPV6 , 2001:c0:a8::x:x
    external-ipblocks 10.0.0.0/8    #IPV6   2001:a:x::x:x
    null-ipblocks     172.16.0.0/13 #IPV6 , 2001:ac:10-17::x:x
end sensor
