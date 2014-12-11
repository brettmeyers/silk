#! /usr/bin/perl -w
# MD5: 7e43712c65063ca2f2dfd3acaf729a09
# TEST: ./rwuniq --pmap-file=../../tests/proto-port-map.pmap --fields=sval --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwuniq --pmap-file=$file{proto_port_map} --fields=sval --sort-output $file{data}";
my $md5 = "7e43712c65063ca2f2dfd3acaf729a09";

check_md5_output($md5, $cmd);
