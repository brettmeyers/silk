#! /usr/bin/perl -w
# MD5: 520cc7e0561cbf55be1e70fa4072e73a
# TEST: ./rwpmaplookup --map-file=../../tests/proto-port-map.pmap --no-title --fields=start-block,end-block,value --no-files 17/0 6/0

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwpmaplookup --map-file=$file{proto_port_map} --no-title --fields=start-block,end-block,value --no-files 17/0 6/0";
my $md5 = "520cc7e0561cbf55be1e70fa4072e73a";

check_md5_output($md5, $cmd);
