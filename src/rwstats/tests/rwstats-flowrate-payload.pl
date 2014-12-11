#! /usr/bin/perl -w
# MD5: c8f56ab0d8ef8174544d5d234702438f
# TEST: ./rwstats --plugin=flowrate.so --fields=payload-bytes --values=bytes,packets,records --count=10 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load flowrate plugin')
    unless check_app_switch($rwstats.' --plugin=flowrate.so', 'fields', qr/payload-rate/);
my $cmd = "$rwstats --plugin=flowrate.so --fields=payload-bytes --values=bytes,packets,records --count=10 $file{data}";
my $md5 = "c8f56ab0d8ef8174544d5d234702438f";

check_md5_output($md5, $cmd);
