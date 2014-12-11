#! /usr/bin/perl -w
# MD5: 9e427174875d8fc14072afabb1a324b5
# TEST: ./rwfilter --pmap-file=../../tests/ip-map.pmap --pmap-any-service-host=dhcp --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwfilter --pmap-file=$file{ip_map} --pmap-any-service-host=dhcp --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "9e427174875d8fc14072afabb1a324b5";

check_md5_output($md5, $cmd);
