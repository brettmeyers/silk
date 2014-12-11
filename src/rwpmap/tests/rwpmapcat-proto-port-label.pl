#! /usr/bin/perl -w
# MD5: 6ef104ae425eeaf0d6ddd5b8ce3e53df
# TEST: ./rwpmapcat --output-type=labels --no-title ../../tests/proto-port-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwpmapcat --output-type=labels --no-title $file{proto_port_map}";
my $md5 = "6ef104ae425eeaf0d6ddd5b8ce3e53df";

check_md5_output($md5, $cmd);
