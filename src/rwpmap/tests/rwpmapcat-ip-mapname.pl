#! /usr/bin/perl -w
# MD5: 82f14a91d95cfbb159cb240198a32a26
# TEST: ./rwpmapcat --output-type=mapname --map-file ../../tests/ip-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmapcat --output-type=mapname --map-file $file{ip_map}";
my $md5 = "82f14a91d95cfbb159cb240198a32a26";

check_md5_output($md5, $cmd);
