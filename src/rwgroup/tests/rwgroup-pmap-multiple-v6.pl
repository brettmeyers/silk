#! /usr/bin/perl -w
# MD5: 3419420e6c3041b892f59c26c02a67a1
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data-v6.rwf | ../rwsort/rwsort --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map-v6.pmap --fields=src-service-host,src-service-port | ./rwgroup --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map-v6.pmap --id-fields=src-service-host,src-service-port | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwfilter = check_silk_app('rwfilter');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
check_features(qw(ipv6));
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{v6data} | $rwsort --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{v6_ip_map} --fields=src-service-host,src-service-port | $rwgroup --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{v6_ip_map} --id-fields=src-service-host,src-service-port | $rwcat --compression-method=none --byte-order=little";
my $md5 = "3419420e6c3041b892f59c26c02a67a1";

check_md5_output($md5, $cmd);
