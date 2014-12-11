#! /usr/bin/perl -w
# MD5: c286f7a17f70e289b8b08af2b0e728dd
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ./rwstats --pmap-file=../../tests/ip-map.pmap --fields=src-service-host --count=10

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwstats --pmap-file=$file{ip_map} --fields=src-service-host --count=10";
my $md5 = "c286f7a17f70e289b8b08af2b0e728dd";

check_md5_output($md5, $cmd);
