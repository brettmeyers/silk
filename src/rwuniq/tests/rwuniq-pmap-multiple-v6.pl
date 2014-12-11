#! /usr/bin/perl -w
# MD5: 4adc04914a38a06edcf3364cf63918ea
# TEST: ./rwuniq --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map-v6.pmap --fields=src-service-host,src-service-port --sort-output ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
check_features(qw(ipv6));
my $cmd = "$rwuniq --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{v6_ip_map} --fields=src-service-host,src-service-port --sort-output $file{v6data}";
my $md5 = "4adc04914a38a06edcf3364cf63918ea";

check_md5_output($md5, $cmd);
