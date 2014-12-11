#! /usr/bin/perl -w
# MD5: a6eacd9b7f2783c5dde924a8ee18dc4b
# TEST: ./rwpmapcat ../../tests/ip-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmapcat $file{ip_map}";
my $md5 = "a6eacd9b7f2783c5dde924a8ee18dc4b";

check_md5_output($md5, $cmd);
