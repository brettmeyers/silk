#! /usr/bin/perl -w
# MD5: c286f7a17f70e289b8b08af2b0e728dd
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data-v6.rwf | ./rwstats --pmap-file=../../tests/ip-map-v6.pmap --fields=src-service-host --count=10

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{v6data} | $rwstats --pmap-file=$file{v6_ip_map} --fields=src-service-host --count=10";
my $md5 = "c286f7a17f70e289b8b08af2b0e728dd";

check_md5_output($md5, $cmd);
