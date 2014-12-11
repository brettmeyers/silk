#! /usr/bin/perl -w
# MD5: 73b140583febf22e9b92e5c8f57e8d58
# TEST: ../rwsort/rwsort --plugin=flowrate.so --fields=pckts/sec ../../tests/data.rwf | ./rwgroup --plugin=flowrate.so --id-fields=pckts/sec | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load flowrate plugin')
    unless check_app_switch($rwsort.' --plugin=flowrate.so', 'fields', qr/payload-rate/);
my $cmd = "$rwsort --plugin=flowrate.so --fields=pckts/sec $file{data} | $rwgroup --plugin=flowrate.so --id-fields=pckts/sec | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "73b140583febf22e9b92e5c8f57e8d58";

check_md5_output($md5, $cmd);
