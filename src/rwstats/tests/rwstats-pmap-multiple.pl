#! /usr/bin/perl -w
# MD5: d5c09b15e9741d4c83758808f7bef1be
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ./rwstats --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map.pmap --fields=src-service-host,src-service-port --count=10

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwstats --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{ip_map} --fields=src-service-host,src-service-port --count=10";
my $md5 = "d5c09b15e9741d4c83758808f7bef1be";

check_md5_output($md5, $cmd);
