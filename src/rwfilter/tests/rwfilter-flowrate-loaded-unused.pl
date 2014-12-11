#! /usr/bin/perl -w
# MD5: 5360ad5a52678d4936e5a83822e86b1a
# TEST: ./rwfilter --plugin=flowrate.so --proto=17 --print-volume-statistics=stdout ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load flowrate plugin')
    unless check_app_switch($rwfilter.' --plugin=flowrate.so', 'payload-rate');
my $cmd = "$rwfilter --plugin=flowrate.so --proto=17 --print-volume-statistics=stdout $file{data}";
my $md5 = "5360ad5a52678d4936e5a83822e86b1a";

check_md5_output($md5, $cmd);
