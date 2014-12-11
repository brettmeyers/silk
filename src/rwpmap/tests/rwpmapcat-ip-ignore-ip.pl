#! /usr/bin/perl -w
# MD5: 5d1aeab6cfbaf0f4eed56549a1dc2454
# TEST: ./rwpmapcat --ip-label-to-ignore=0.0.0.0 ../../tests/ip-map.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmapcat --ip-label-to-ignore=0.0.0.0 $file{ip_map}";
my $md5 = "5d1aeab6cfbaf0f4eed56549a1dc2454";

check_md5_output($md5, $cmd);
