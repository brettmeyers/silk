#! /usr/bin/perl -w
# MD5: ffe28be2b01eef83dc8a34a5fb6c3ba3
# TEST: ./rwpmapcat --no-cidr-blocks ../../tests/ip-map-v6.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwpmapcat --no-cidr-blocks $file{v6_ip_map}";
my $md5 = "ffe28be2b01eef83dc8a34a5fb6c3ba3";

check_md5_output($md5, $cmd);
