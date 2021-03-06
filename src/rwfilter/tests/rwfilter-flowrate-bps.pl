#! /usr/bin/perl -w
# MD5: df3226e49dc768f0afce6b69f97e07f4
# TEST: ./rwfilter --plugin=flowrate.so --bytes-per-second=100- --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load flowrate plugin')
    unless check_app_switch($rwfilter.' --plugin=flowrate.so', 'payload-rate');
my $cmd = "$rwfilter --plugin=flowrate.so --bytes-per-second=100- --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "df3226e49dc768f0afce6b69f97e07f4";

check_md5_output($md5, $cmd);
