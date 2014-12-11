#! /usr/bin/perl -w
# MD5: 2deaf642de2cf0c5cbadd8ec373c2322
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ./rwsort --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map.pmap --fields=src-service-host,src-service-port | ../rwuniq/rwuniq --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map.pmap --fields=src-service-host,src-service-port --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwfilter = check_silk_app('rwfilter');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwsort --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{ip_map} --fields=src-service-host,src-service-port | $rwuniq --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{ip_map} --fields=src-service-host,src-service-port --presorted-input";
my $md5 = "2deaf642de2cf0c5cbadd8ec373c2322";

check_md5_output($md5, $cmd);
