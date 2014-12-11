#! /usr/bin/perl -w
# MD5: 0a86c69837180d18cdcb3f646dc2a709
# TEST: ./rwsort --plugin=flowrate.so --fields=pckts/sec ../../tests/data.rwf | ../rwuniq/rwuniq --plugin=flowrate.so --fields=pckts/sec --values=packets --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load flowrate plugin')
    unless check_app_switch($rwsort.' --plugin=flowrate.so', 'fields', qr/payload-rate/);
my $cmd = "$rwsort --plugin=flowrate.so --fields=pckts/sec $file{data} | $rwuniq --plugin=flowrate.so --fields=pckts/sec --values=packets --presorted-input";
my $md5 = "0a86c69837180d18cdcb3f646dc2a709";

check_md5_output($md5, $cmd);
