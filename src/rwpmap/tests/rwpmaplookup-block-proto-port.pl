#! /usr/bin/perl -w
# MD5: b9dcea875459c65a995fb546750964fc
# TEST: ./rwpmaplookup --map-file=../../tests/proto-port-map.pmap --no-title --fields=block,key,value --no-files 17/0 6/0

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwpmaplookup --map-file=$file{proto_port_map} --no-title --fields=block,key,value --no-files 17/0 6/0";
my $md5 = "b9dcea875459c65a995fb546750964fc";

check_md5_output($md5, $cmd);
