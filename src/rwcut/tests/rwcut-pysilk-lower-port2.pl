#! /usr/bin/perl -w
# MD5: 109574562fd09e534f968c19a38303f6
# TEST: ./rwcut --python-file=../../tests/pysilk-plugin.py --fields=lower_port,lower_port ../../tests/data.rwf

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
my $cmd = "$rwcut --python-file=$file{pysilk_plugin} --fields=lower_port,lower_port $file{data}";
my $md5 = "109574562fd09e534f968c19a38303f6";

check_md5_output($md5, $cmd);
