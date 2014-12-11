#! /usr/bin/perl -w
# MD5: 1c0632eb6be72e2a09447a7696795dd8
# TEST: ./rwsort --python-file=../../tests/pysilk-plugin.py --fields=proto_name ../../tests/data.rwf | ../rwuniq/rwuniq --python-file=../../tests/pysilk-plugin.py --fields=proto_name --values=bytes --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{pysilk_plugin} = get_data_or_exit77('pysilk_plugin');
$ENV{PYTHONPATH} = $SiLKTests::testsdir.((defined $ENV{PYTHONPATH}) ? ":$ENV{PYTHONPATH}" : "");
add_plugin_dirs('/src/pysilk');

skip_test('Cannot use --python-file') 
    unless check_exit_status(qq|$rwsort --python-file=$file{pysilk_plugin} --help|);
my $cmd = "$rwsort --python-file=$file{pysilk_plugin} --fields=proto_name $file{data} | $rwuniq --python-file=$file{pysilk_plugin} --fields=proto_name --values=bytes --presorted-input";
my $md5 = "1c0632eb6be72e2a09447a7696795dd8";

check_md5_output($md5, $cmd);
