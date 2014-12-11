#! /usr/bin/perl -w
# MD5: 44a8335d9a2f9e3c170f987db84ecf2c
# TEST: ./rwsort --python-file=../../tests/pysilk-plugin.py --fields=server_ipv6 ../../tests/data-v6.rwf | ../rwuniq/rwuniq --python-file=../../tests/pysilk-plugin.py --fields=server_ipv6 --values=bytes --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{pysilk_plugin} = get_data_or_exit77('pysilk_plugin');
$ENV{PYTHONPATH} = $SiLKTests::testsdir.((defined $ENV{PYTHONPATH}) ? ":$ENV{PYTHONPATH}" : "");
add_plugin_dirs('/src/pysilk');
check_features(qw(ipv6));

skip_test('Cannot use --python-file') 
    unless check_exit_status(qq|$rwsort --python-file=$file{pysilk_plugin} --help|);
my $cmd = "$rwsort --python-file=$file{pysilk_plugin} --fields=server_ipv6 $file{v6data} | $rwuniq --python-file=$file{pysilk_plugin} --fields=server_ipv6 --values=bytes --presorted-input";
my $md5 = "44a8335d9a2f9e3c170f987db84ecf2c";

check_md5_output($md5, $cmd);
