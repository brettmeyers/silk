#! /usr/bin/perl -w
# MD5: d961793510102ef8801d9c37d710e968
# TEST: ./rwcut --pmap-file=../../tests/proto-port-map.pmap --fields=sval,dval ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwcut --pmap-file=$file{proto_port_map} --fields=sval,dval $file{data}";
my $md5 = "d961793510102ef8801d9c37d710e968";

check_md5_output($md5, $cmd);
