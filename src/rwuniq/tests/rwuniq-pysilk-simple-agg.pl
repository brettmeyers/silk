#! /usr/bin/perl -w
# MD5: 531ffd2cd2a4d0becddaaa7c0a31e7ca
# TEST: ./rwuniq --python-file=../../tests/pysilk-plugin.py --fields=lower_port_simple --values=large_packet_flows,largest_packets,smallest_packets --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{pysilk_plugin} = get_data_or_exit77('pysilk_plugin');
$ENV{PYTHONPATH} = $SiLKTests::testsdir.((defined $ENV{PYTHONPATH}) ? ":$ENV{PYTHONPATH}" : "");
add_plugin_dirs('/src/pysilk');

skip_test('Cannot use --python-file') 
    unless check_exit_status(qq|$rwuniq --python-file=$file{pysilk_plugin} --help|);
my $cmd = "$rwuniq --python-file=$file{pysilk_plugin} --fields=lower_port_simple --values=large_packet_flows,largest_packets,smallest_packets --sort-output $file{data}";
my $md5 = "531ffd2cd2a4d0becddaaa7c0a31e7ca";

check_md5_output($md5, $cmd);
