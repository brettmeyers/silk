#! /usr/bin/perl -w
# MD5: 309c87650f4629f4b0fccdb79419a5c4
# TEST: ./rwcut --python-file=../../tests/pysilk-plugin.py --fields=sip,dip,sport,dport,server_ipv6 --num-recs=10000 ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{pysilk_plugin} = get_data_or_exit77('pysilk_plugin');
$ENV{PYTHONPATH} = $SiLKTests::testsdir.((defined $ENV{PYTHONPATH}) ? ":$ENV{PYTHONPATH}" : "");
add_plugin_dirs('/src/pysilk');
check_features(qw(ipv6));

skip_test('Cannot use --python-file') 
    unless check_exit_status(qq|$rwcut --python-file=$file{pysilk_plugin} --help|);
my $cmd = "$rwcut --python-file=$file{pysilk_plugin} --fields=sip,dip,sport,dport,server_ipv6 --num-recs=10000 $file{v6data}";
my $md5 = "309c87650f4629f4b0fccdb79419a5c4";

check_md5_output($md5, $cmd);
