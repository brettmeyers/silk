#! /usr/bin/perl -w
# MD5: 109aae4e122fdd7a798ac2dd7a2e8596
# TEST: ./rwpmapcat --output-type=type --no-titles ../../tests/ip-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmapcat --output-type=type --no-titles $file{ip_map}";
my $md5 = "109aae4e122fdd7a798ac2dd7a2e8596";

check_md5_output($md5, $cmd);
