#! /usr/bin/perl -w
# MD5: 6cf5e1fed70fa4ef82804392913909b9
# TEST: ./rwcut --python-file=../../tests/pysilk-plugin.py --fields=sip,dip,server_ip,sport,dport,lower_port_simple,protocol,proto_name --num-recs=10000 --delimited=, ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
$file{pysilk_plugin} = get_data_or_exit77('pysilk_plugin');
$ENV{PYTHONPATH} = $SiLKTests::testsdir.((defined $ENV{PYTHONPATH}) ? ":$ENV{PYTHONPATH}" : "");
add_plugin_dirs('/src/pysilk');

skip_test('Cannot use --python-file') 
    unless check_exit_status(qq|$rwcut --python-file=$file{pysilk_plugin} --help|);
my $cmd = "$rwcut --python-file=$file{pysilk_plugin} --fields=sip,dip,server_ip,sport,dport,lower_port_simple,protocol,proto_name --num-recs=10000 --delimited=, $file{data}";
my $md5 = "6cf5e1fed70fa4ef82804392913909b9";

check_md5_output($md5, $cmd);
