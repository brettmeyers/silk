#! /usr/bin/perl -w
# MD5: 0ce7fcdf6722144052f4558bdf81f3bb
# TEST: ./rwpmapcat ../../tests/ip-map-v6.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwpmapcat $file{v6_ip_map}";
my $md5 = "0ce7fcdf6722144052f4558bdf81f3bb";

check_md5_output($md5, $cmd);
