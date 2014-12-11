#! /usr/bin/perl -w
# MD5: 0647abf67bceb044255aa526c55a6ea4
# TEST: ./rwfilter --python-expr='rec.sport==rec.dport' --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

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
my $cmd = "$rwfilter --python-expr='rec.sport==rec.dport' --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "0647abf67bceb044255aa526c55a6ea4";

check_md5_output($md5, $cmd);
