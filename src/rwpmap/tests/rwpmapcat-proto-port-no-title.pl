#! /usr/bin/perl -w
# MD5: 8ab9e63d65dcc15bf6909f8bcf02695a
# TEST: ./rwpmapcat --no-titles ../../tests/proto-port-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{proto_port_map} = get_data_or_exit77('proto_port_map');
my $cmd = "$rwpmapcat --no-titles $file{proto_port_map}";
my $md5 = "8ab9e63d65dcc15bf6909f8bcf02695a";

check_md5_output($md5, $cmd);
