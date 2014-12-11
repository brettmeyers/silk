#! /usr/bin/perl -w
# MD5: 3b4610b6f9627bd1b2f9d2d8fa9a6071
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data-v6.rwf | ./rwstats --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map-v6.pmap --fields=src-service-host,src-service-port --count=10

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
check_features(qw(ipv6));
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{v6data} | $rwstats --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{v6_ip_map} --fields=src-service-host,src-service-port --count=10";
my $md5 = "3b4610b6f9627bd1b2f9d2d8fa9a6071";

check_md5_output($md5, $cmd);
