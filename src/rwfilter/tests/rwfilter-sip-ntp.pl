#! /usr/bin/perl -w
# MD5: 97e5352a76ce1b91e188a7eee4f4b3c5
# TEST: ./rwfilter --pmap-file=../../tests/ip-map.pmap --pmap-src-service-host=ntp --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwfilter --pmap-file=$file{ip_map} --pmap-src-service-host=ntp --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "97e5352a76ce1b91e188a7eee4f4b3c5";

check_md5_output($md5, $cmd);
