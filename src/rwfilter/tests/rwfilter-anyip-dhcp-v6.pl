#! /usr/bin/perl -w
# MD5: 87ec873e03d3a8baaee8cb0919c8525b
# TEST: ./rwfilter --pmap-file=../../tests/ip-map-v6.pmap --pmap-any-service-host=dhcp --pass=stdout ../../tests/data-v6.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwfilter --pmap-file=$file{v6_ip_map} --pmap-any-service-host=dhcp --pass=stdout $file{v6data} | $rwcat --compression-method=none --byte-order=little";
my $md5 = "87ec873e03d3a8baaee8cb0919c8525b";

check_md5_output($md5, $cmd);
