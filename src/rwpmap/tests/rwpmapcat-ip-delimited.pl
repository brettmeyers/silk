#! /usr/bin/perl -w
# MD5: a3b70e82b28d37c55896e780250351d5
# TEST: ./rwpmapcat --delimited=, --no-cidr --map-file ../../tests/ip-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmapcat --delimited=, --no-cidr --map-file $file{ip_map}";
my $md5 = "a3b70e82b28d37c55896e780250351d5";

check_md5_output($md5, $cmd);
