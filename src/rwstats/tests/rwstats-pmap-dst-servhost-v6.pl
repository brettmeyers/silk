#! /usr/bin/perl -w
# MD5: dd3ad4d4228dd4e4a68d3c46e58aa06f
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data-v6.rwf | ./rwstats --pmap-file=servhost:../../tests/ip-map-v6.pmap --fields=dst-servhost --count=10

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{v6data} | $rwstats --pmap-file=servhost:$file{v6_ip_map} --fields=dst-servhost --count=10";
my $md5 = "dd3ad4d4228dd4e4a68d3c46e58aa06f";

check_md5_output($md5, $cmd);
