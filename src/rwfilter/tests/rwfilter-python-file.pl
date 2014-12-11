#! /usr/bin/perl -w
# MD5: 97951e047345b07e92c9ccb93fe55021
# TEST: ./rwfilter --python-file=../../tests/pysilk-plugin.py --print-volume ../../tests/data.rwf 2>&1

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{pysilk_plugin} = get_data_or_exit77('pysilk_plugin');
$ENV{PYTHONPATH} = $SiLKTests::testsdir.((defined $ENV{PYTHONPATH}) ? ":$ENV{PYTHONPATH}" : "");
add_plugin_dirs('/src/pysilk');

skip_test('Cannot use --python-file') 
    unless check_exit_status(qq|$rwfilter --python-file=$file{pysilk_plugin} --help|);
my $cmd = "$rwfilter --python-file=$file{pysilk_plugin} --print-volume $file{data} 2>&1";
my $md5 = "97951e047345b07e92c9ccb93fe55021";

check_md5_output($md5, $cmd);
