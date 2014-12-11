#! /usr/bin/perl -w
# MD5: 53ff77183be9d43fb588c46396aabc15
# TEST: ./rwpmapcat --column-sep=, --map-file=../../tests/proto-port-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwpmapcat --column-sep=, --map-file=$file{proto_port_map}";
my $md5 = "53ff77183be9d43fb588c46396aabc15";

check_md5_output($md5, $cmd);
