#! /usr/bin/perl -w
# MD5: 402114d5a7b5b213c9910ac0cb0cf35e
# TEST: ../rwsort/rwsort --python-file=../../tests/pysilk-plugin.py --fields=lower_port ../../tests/data.rwf | ./rwgroup --python-file=../../tests/pysilk-plugin.py --id-fields=lower_port | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
$file{pysilk_plugin} = get_data_or_exit77('pysilk_plugin');
$ENV{PYTHONPATH} = $SiLKTests::testsdir.((defined $ENV{PYTHONPATH}) ? ":$ENV{PYTHONPATH}" : "");
add_plugin_dirs('/src/pysilk');

skip_test('Cannot use --python-file') 
    unless check_exit_status(qq|$rwgroup --python-file=$file{pysilk_plugin} --help|);
my $cmd = "$rwsort --python-file=$file{pysilk_plugin} --fields=lower_port $file{data} | $rwgroup --python-file=$file{pysilk_plugin} --id-fields=lower_port | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "402114d5a7b5b213c9910ac0cb0cf35e";

check_md5_output($md5, $cmd);
