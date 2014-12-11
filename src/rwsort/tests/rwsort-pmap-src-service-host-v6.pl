#! /usr/bin/perl -w
# MD5: 2659bb4e737b06dc8fd56ab892975f15
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data-v6.rwf | ./rwsort --pmap-file=../../tests/ip-map-v6.pmap --fields=src-service-host | ../rwuniq/rwuniq --pmap-file=../../tests/ip-map-v6.pmap --fields=src-service-host --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwfilter = check_silk_app('rwfilter');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{v6data} | $rwsort --pmap-file=$file{v6_ip_map} --fields=src-service-host | $rwuniq --pmap-file=$file{v6_ip_map} --fields=src-service-host --presorted-input";
my $md5 = "2659bb4e737b06dc8fd56ab892975f15";

check_md5_output($md5, $cmd);
