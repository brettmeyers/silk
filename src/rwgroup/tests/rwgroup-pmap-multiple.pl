#! /usr/bin/perl -w
# MD5: 07dbd25a9310d1280624b40d20d22133
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ../rwsort/rwsort --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map.pmap --fields=src-service-host,src-service-port | ./rwgroup --pmap-file=service-port:../../tests/proto-port-map.pmap --pmap-file=../../tests/ip-map.pmap --id-fields=src-service-host,src-service-port | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwfilter = check_silk_app('rwfilter');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwsort --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{ip_map} --fields=src-service-host,src-service-port | $rwgroup --pmap-file=service-port:$file{proto_port_map} --pmap-file=$file{ip_map} --id-fields=src-service-host,src-service-port | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "07dbd25a9310d1280624b40d20d22133";

check_md5_output($md5, $cmd);
