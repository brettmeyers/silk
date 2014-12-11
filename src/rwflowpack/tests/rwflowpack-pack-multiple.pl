#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: rwflowpack-pack-multiple.pl 40a363507ed0 2014-04-01 14:09:52Z mthomas $")

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

# send data to these ports and host
my $host = '127.0.0.1';
my $port1 = get_ephemeral_port($host, 'udp');
my $port2 = get_ephemeral_port($host, 'udp');

# Generate the sensor.conf file
my $sensor_conf = "$tmpdir/sensor-templ.conf";
{
    # undef record separator to slurp all of <DATA> into variable
    local $/;
    my $sensor_conf_text = <DATA>;
    $sensor_conf_text =~ s,\$\{host\},$host,g;
    $sensor_conf_text =~ s,\$\{port1\},$port1,g;
    $sensor_conf_text =~ s,\$\{port2\},$port2,g;
    make_config_file($sensor_conf, \$sensor_conf_text);
}

# the command that wraps rwflowpack
my $cmd = join " ", ("$SiLKTests::PYTHON $srcdir/tests/rwflowpack-daemon.py",
                     ($ENV{SK_TESTS_VERBOSE} ? "--verbose" : ()),
                     ($ENV{SK_TESTS_LOG_DEBUG} ? "--log-level=debug" : ()),
                     "--sensor-conf=$sensor_conf",
                     "--copy $file{data}:incoming",
                     "--copy $file{data}:incoming2",
                     "--pdu 40000,$host,$port1",
                     "--pdu 40000,$host,$port2",
                     "--limit=1083752",
                     "--basedir=$tmpdir",
                     "--daemon-timeout=90",
                     "--",
                     "--polling-interval=5",
    );

# run it and check the MD5 hash of its output
check_md5_output('582c3cb2df3327fcfeff02854c96610e', $cmd);


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
check_md5_output('06ee2be63885cc484d918f1d1cdf7584', $cmd);

# successful!
exit 0;

__DATA__
# the sensor.conf file for this test
probe P0-silk silk
    poll-directory ${incoming}
end probe

probe P1 silk
    poll-directory ${incoming2}
end probe

probe P0-pdu netflow-v5
    listen-on-port ${port1}
    protocol udp
    listen-as-host ${host}
    accept-from-host ${host}
end probe

probe P2 netflow-v5
    listen-on-port ${port2}
    protocol udp
    listen-as-host ${host}
    accept-from-host ${host}
end probe

# sensor S0 is made up of two probes
sensor S0
    silk-probes P0-silk
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
