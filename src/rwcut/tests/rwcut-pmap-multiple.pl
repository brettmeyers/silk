#! /usr/bin/perl -w
# MD5: 6b8bba07ec4e64659b747465b959d9e4
# TEST: ./rwcut --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map.pmap --fields=src-service-host,src-service-port,src-service-host,src-service-port ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwcut --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{ip_map} --fields=src-service-host,src-service-port,src-service-host,src-service-port $file{data}";
my $md5 = "6b8bba07ec4e64659b747465b959d9e4";

check_md5_output($md5, $cmd);
