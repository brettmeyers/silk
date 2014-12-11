#! /usr/bin/perl -w
# MD5: 195d5ed54f22a6beaae1e934040212fc
# TEST: ./rwfilter --plugin=flowrate.so --packets-per-second=100-1000 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load flowrate plugin')
    unless check_app_switch($rwfilter.' --plugin=flowrate.so', 'payload-rate');
my $cmd = "$rwfilter --plugin=flowrate.so --packets-per-second=100-1000 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "195d5ed54f22a6beaae1e934040212fc";

check_md5_output($md5, $cmd);
