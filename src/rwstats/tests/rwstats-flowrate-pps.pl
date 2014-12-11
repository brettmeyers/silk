#! /usr/bin/perl -w
# MD5: 8f3baa8ac2643c34b29d297fbf78cb65
# TEST: ./rwstats --plugin=flowrate.so --fields=pckts/sec --values=packets --count=10 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load flowrate plugin')
    unless check_app_switch($rwstats.' --plugin=flowrate.so', 'fields', qr/payload-rate/);
my $cmd = "$rwstats --plugin=flowrate.so --fields=pckts/sec --values=packets --count=10 $file{data}";
my $md5 = "8f3baa8ac2643c34b29d297fbf78cb65";

check_md5_output($md5, $cmd);
