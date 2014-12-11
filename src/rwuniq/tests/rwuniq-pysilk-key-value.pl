#! /usr/bin/perl -w
# MD5: 2bb554812a975058458d1ee97abb8666
# TEST: ./rwuniq --python-file=../../tests/pysilk-plugin.py --fields=lower_port --values=max_bytes --sort-output ../../tests/data.rwf

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
my $cmd = "$rwuniq --python-file=$file{pysilk_plugin} --fields=lower_port --values=max_bytes --sort-output $file{data}";
my $md5 = "2bb554812a975058458d1ee97abb8666";

check_md5_output($md5, $cmd);
