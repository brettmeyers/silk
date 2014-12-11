#! /usr/bin/perl -w
# MD5: 3e4a24a4c579adabffd200ced724ffba
# TEST: ./rwpmapcat --output-type=mapname ../../tests/proto-port-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwpmapcat --output-type=mapname $file{proto_port_map}";
my $md5 = "3e4a24a4c579adabffd200ced724ffba";

check_md5_output($md5, $cmd);
