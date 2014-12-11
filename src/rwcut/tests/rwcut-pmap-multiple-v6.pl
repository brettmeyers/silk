#! /usr/bin/perl -w
# MD5: 424285e04a286e095608e5d6c162d323
# TEST: ./rwcut --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map-v6.pmap --fields=src-service-host,src-service-port,src-service-host,src-service-port ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
check_features(qw(ipv6));
my $cmd = "$rwcut --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{v6_ip_map} --fields=src-service-host,src-service-port,src-service-host,src-service-port $file{v6data}";
my $md5 = "424285e04a286e095608e5d6c162d323";

check_md5_output($md5, $cmd);
