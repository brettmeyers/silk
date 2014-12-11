#! /usr/bin/perl -w
# MD5: 2e47bb46e9c387ec7c6b7c52447bf4cd
# TEST: ./rwuniq --plugin=flowrate.so --fields=payload-bytes --values=bytes,packets,records --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load flowrate plugin')
    unless check_app_switch($rwuniq.' --plugin=flowrate.so', 'fields', qr/payload-rate/);
my $cmd = "$rwuniq --plugin=flowrate.so --fields=payload-bytes --values=bytes,packets,records --sort-output $file{data}";
my $md5 = "2e47bb46e9c387ec7c6b7c52447bf4cd";

check_md5_output($md5, $cmd);
