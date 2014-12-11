#! /usr/bin/perl -w
# MD5: 7a445c5ce3f2eb05e767badf79c2c854
# TEST: ./rwpmapcat --ip-label-to-ignore=:: ../../tests/ip-map-v6.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwpmapcat --ip-label-to-ignore=:: $file{v6_ip_map}";
my $md5 = "7a445c5ce3f2eb05e767badf79c2c854";

check_md5_output($md5, $cmd);
