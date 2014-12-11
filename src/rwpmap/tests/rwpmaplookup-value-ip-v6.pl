#! /usr/bin/perl -w
# MD5: 5e91da9822234b33b8affd196936a178
# TEST: ./rwpmaplookup --map-file=../../tests/ip-map-v6.pmap --fields=value --no-title -delim --no-files 2001:db8:ac:18::ba:d

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwpmaplookup --map-file=$file{v6_ip_map} --fields=value --no-title -delim --no-files 2001:db8:ac:18::ba:d";
my $md5 = "5e91da9822234b33b8affd196936a178";

check_md5_output($md5, $cmd);
