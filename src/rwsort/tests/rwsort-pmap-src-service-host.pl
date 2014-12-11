#! /usr/bin/perl -w
# MD5: 2659bb4e737b06dc8fd56ab892975f15
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ./rwsort --pmap-file=../../tests/ip-map.pmap --fields=src-service-host | ../rwuniq/rwuniq --pmap-file=../../tests/ip-map.pmap --fields=src-service-host --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwfilter = check_silk_app('rwfilter');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwsort --pmap-file=$file{ip_map} --fields=src-service-host | $rwuniq --pmap-file=$file{ip_map} --fields=src-service-host --presorted-input";
my $md5 = "2659bb4e737b06dc8fd56ab892975f15";

check_md5_output($md5, $cmd);
