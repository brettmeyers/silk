#! /usr/bin/perl -w
# MD5: c21124743fe17d1a3fc6622511035ca9
# TEST: ./rwpmapcat --output-type=type,mapname --map-file=../../tests/proto-port-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwpmapcat --output-type=type,mapname --map-file=$file{proto_port_map}";
my $md5 = "c21124743fe17d1a3fc6622511035ca9";

check_md5_output($md5, $cmd);
