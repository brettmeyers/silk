#! /usr/bin/perl -w
# MD5: 551a66ff434777c7c0393b030fde5530
# TEST: ./rwstats --python-file=../../tests/pysilk-plugin.py --fields=lower_port --values=max_bytes --count=10 --no-percent ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
$file{pysilk_plugin} = get_data_or_exit77('pysilk_plugin');
$ENV{PYTHONPATH} = $SiLKTests::testsdir.((defined $ENV{PYTHONPATH}) ? ":$ENV{PYTHONPATH}" : "");
add_plugin_dirs('/src/pysilk');

skip_test('Cannot use --python-file') 
    unless check_exit_status(qq|$rwstats --python-file=$file{pysilk_plugin} --help|);
my $cmd = "$rwstats --python-file=$file{pysilk_plugin} --fields=lower_port --values=max_bytes --count=10 --no-percent $file{data}";
my $md5 = "551a66ff434777c7c0393b030fde5530";

check_md5_output($md5, $cmd);
