#! /usr/bin/perl -w
# MD5: 54436bc7690728a9154a04f047ae1231
# TEST: ./rwstats --python-file=../../tests/pysilk-plugin.py --fields=lower_port_simple --values=large_packet_flows,largest_packets,smallest_packets --count=5 --no-percent ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
$file{pysilk_plugin} = get_data_or_exit77('pysilk_plugin');
$ENV{PYTHONPATH} = $SiLKTests::testsdir.((defined $ENV{PYTHONPATH}) ? ":$ENV{PYTHONPATH}" : "");
add_plugin_dirs('/src/pysilk');

skip_test('Cannot use --python-file') 
    unless check_exit_status(qq|$rwstats --python-file=$file{pysilk_plugin} --help|);
my $cmd = "$rwstats --python-file=$file{pysilk_plugin} --fields=lower_port_simple --values=large_packet_flows,largest_packets,smallest_packets --count=5 --no-percent $file{data}";
my $md5 = "54436bc7690728a9154a04f047ae1231";

check_md5_output($md5, $cmd);
