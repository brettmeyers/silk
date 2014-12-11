#! /usr/bin/perl -w
# MD5: 5360ad5a52678d4936e5a83822e86b1a
# TEST: ./rwfilter --proto=17 --print-volume-statistics=stdout ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
$file{pysilk_plugin} = get_data_or_exit77('pysilk_plugin');
$ENV{PYTHONPATH} = $SiLKTests::testsdir.((defined $ENV{PYTHONPATH}) ? ":$ENV{PYTHONPATH}" : "");
add_plugin_dirs('/src/pysilk');

skip_test('Cannot use --python-file') 
    unless check_exit_status(qq|$rwfilter --python-file=$file{pysilk_plugin} --help|);
my $cmd = "$rwfilter --proto=17 --print-volume-statistics=stdout $file{data}";
my $md5 = "5360ad5a52678d4936e5a83822e86b1a";

check_md5_output($md5, $cmd);
