#! /usr/bin/perl -w
# MD5: 714adadf676c01e5c68005eec28a4df6
# TEST: ./rwpmaplookup --map-file=../../tests/ip-map-v6.pmap --no-title --fields=start-block,end-block,value --no-files 2001:db8:ac:10::11:12 2001:db8:ac:1e::1f:20

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwpmaplookup --map-file=$file{v6_ip_map} --no-title --fields=start-block,end-block,value --no-files 2001:db8:ac:10::11:12 2001:db8:ac:1e::1f:20";
my $md5 = "714adadf676c01e5c68005eec28a4df6";

check_md5_output($md5, $cmd);
