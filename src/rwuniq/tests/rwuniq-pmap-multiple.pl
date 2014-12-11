#! /usr/bin/perl -w
# MD5: c5728c6e8134d2c62d9993cb24c32773
# TEST: ./rwuniq --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map.pmap --fields=src-service-host,src-service-port --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwuniq --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{ip_map} --fields=src-service-host,src-service-port --sort-output $file{data}";
my $md5 = "c5728c6e8134d2c62d9993cb24c32773";

check_md5_output($md5, $cmd);
