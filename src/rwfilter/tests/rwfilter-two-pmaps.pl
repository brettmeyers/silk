#! /usr/bin/perl -w
# MD5: 32a4b25e26c11a4b270f3a135f3cf901
# TEST: ./rwfilter --pmap-file=service:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map.pmap --pmap-any-service=UDP/NTP --pmap-any-service-host=ntp --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwfilter --pmap-file=service:$file{proto_port_map} --pmap-file=$file{ip_map} --pmap-any-service=UDP/NTP --pmap-any-service-host=ntp --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "32a4b25e26c11a4b270f3a135f3cf901";

check_md5_output($md5, $cmd);
