#! /usr/bin/perl -w
# MD5: 2ea0d70400291589bd4f299379550397
# TEST: ./rwcut --python-file=../../tests/pysilk-plugin.py --fields=scc,py-scc,dcc,py-dcc --num-recs=10000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
$file{pysilk_plugin} = get_data_or_exit77('pysilk_plugin');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{PYTHONPATH} = $SiLKTests::testsdir.((defined $ENV{PYTHONPATH}) ? ":$ENV{PYTHONPATH}" : "");
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
add_plugin_dirs('/src/pysilk');

skip_test('Cannot use --python-file') 
    unless check_exit_status(qq|$rwcut --python-file=$file{pysilk_plugin} --help|);
my $cmd = "$rwcut --python-file=$file{pysilk_plugin} --fields=scc,py-scc,dcc,py-dcc --num-recs=10000 $file{data}";
my $md5 = "2ea0d70400291589bd4f299379550397";

check_md5_output($md5, $cmd);
