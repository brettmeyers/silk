#! /usr/bin/perl -w
# MD5: 158490c17bccd66f30a3287ba7fb22a1
# TEST: ./rwpmapcat --ignore-label=unknown ../../tests/proto-port-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwpmapcat --ignore-label=unknown $file{proto_port_map}";
my $md5 = "158490c17bccd66f30a3287ba7fb22a1";

check_md5_output($md5, $cmd);
