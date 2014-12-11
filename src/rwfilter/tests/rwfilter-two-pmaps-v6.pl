#! /usr/bin/perl -w
# MD5: 320113d773719765572578936fae817b
# TEST: ./rwfilter --pmap-file=service:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map-v6.pmap --pmap-any-service=UDP/NTP --pmap-any-service-host=ntp --pass=stdout ../../tests/data-v6.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
check_features(qw(ipv6));
my $cmd = "$rwfilter --pmap-file=service:$file{proto_port_map} --pmap-file=$file{v6_ip_map} --pmap-any-service=UDP/NTP --pmap-any-service-host=ntp --pass=stdout $file{v6data} | $rwcat --compression-method=none --byte-order=little";
my $md5 = "320113d773719765572578936fae817b";

check_md5_output($md5, $cmd);
