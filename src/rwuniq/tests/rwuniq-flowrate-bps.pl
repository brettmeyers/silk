#! /usr/bin/perl -w
# MD5: 32d68f3cadd8e056a0b2c57bdd61058d
# TEST: ./rwuniq --plugin=flowrate.so --fields=bytes/sec --values=bytes --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load flowrate plugin')
    unless check_app_switch($rwuniq.' --plugin=flowrate.so', 'fields', qr/payload-rate/);
my $cmd = "$rwuniq --plugin=flowrate.so --fields=bytes/sec --values=bytes --sort-output $file{data}";
my $md5 = "32d68f3cadd8e056a0b2c57bdd61058d";

check_md5_output($md5, $cmd);
