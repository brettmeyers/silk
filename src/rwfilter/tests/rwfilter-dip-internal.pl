#! /usr/bin/perl -w
# MD5: 68a6395b288dd1c6fc19144a55f9f47e
# TEST: ./rwfilter --pmap-file=../../tests/ip-map.pmap --pmap-dst-service-host='internal,internal services' --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwfilter --pmap-file=$file{ip_map} --pmap-dst-service-host='internal,internal services' --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "68a6395b288dd1c6fc19144a55f9f47e";

check_md5_output($md5, $cmd);
