#! /usr/bin/perl -w
# MD5: 610e7989e8feac31ba2a3a62e8dc1c29
# TEST: ./rwpmaplookup --map-file=../../tests/proto-port-map.pmap --no-title --no-files 17/67

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwpmaplookup --map-file=$file{proto_port_map} --no-title --no-files 17/67";
my $md5 = "610e7989e8feac31ba2a3a62e8dc1c29";

check_md5_output($md5, $cmd);
