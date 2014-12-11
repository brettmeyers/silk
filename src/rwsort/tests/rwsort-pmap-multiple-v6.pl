#! /usr/bin/perl -w
# MD5: 940cd6abc01daa5dfc3cdeb4c5260eae
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data-v6.rwf | ./rwsort --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map-v6.pmap --fields=src-service-host,src-service-port | ../rwuniq/rwuniq --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map-v6.pmap --fields=src-service-host,src-service-port --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwfilter = check_silk_app('rwfilter');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
check_features(qw(ipv6));
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{v6data} | $rwsort --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{v6_ip_map} --fields=src-service-host,src-service-port | $rwuniq --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{v6_ip_map} --fields=src-service-host,src-service-port --presorted-input";
my $md5 = "940cd6abc01daa5dfc3cdeb4c5260eae";

check_md5_output($md5, $cmd);
