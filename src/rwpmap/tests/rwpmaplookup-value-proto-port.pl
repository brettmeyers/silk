#! /usr/bin/perl -w
# MD5: 7521697228ea957fd84ad057e129ab37
# TEST: ./rwpmaplookup --map-file=../../tests/proto-port-map.pmap --fields=value --no-title -delim --no-files 17/67

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwpmaplookup --map-file=$file{proto_port_map} --fields=value --no-title -delim --no-files 17/67";
my $md5 = "7521697228ea957fd84ad057e129ab37";

check_md5_output($md5, $cmd);
