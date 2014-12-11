#! /usr/bin/perl -w
# MD5: 9a6e36e3780e4686b6e4bf9b056f06f4
# TEST: ./rwfilter --pmap-file=../../tests/ip-map-v6.pmap --pmap-src-service-host=ntp --pass=stdout ../../tests/data-v6.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwfilter --pmap-file=$file{v6_ip_map} --pmap-src-service-host=ntp --pass=stdout $file{v6data} | $rwcat --compression-method=none --byte-order=little";
my $md5 = "9a6e36e3780e4686b6e4bf9b056f06f4";

check_md5_output($md5, $cmd);
