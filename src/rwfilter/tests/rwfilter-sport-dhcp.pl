#! /usr/bin/perl -w
# MD5: 9e427174875d8fc14072afabb1a324b5
# TEST: ./rwfilter --pmap-file=../../tests/proto-port-map.pmap --pmap-sport-proto=UDP/DHCP --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwfilter --pmap-file=$file{proto_port_map} --pmap-sport-proto=UDP/DHCP --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "9e427174875d8fc14072afabb1a324b5";

check_md5_output($md5, $cmd);
