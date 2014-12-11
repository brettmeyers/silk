#! /usr/bin/perl -w
# MD5: 63d064e4b9382f1d99bd4a0b6fff813c
# TEST: ./rwpmapcat --output-type=labels --map-file ../../tests/ip-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmapcat --output-type=labels --map-file $file{ip_map}";
my $md5 = "63d064e4b9382f1d99bd4a0b6fff813c";

check_md5_output($md5, $cmd);
