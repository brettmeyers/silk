#! /usr/bin/perl -w
# MD5: 44efab1abb941a646df2bbfe4d2b0550
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ../rwsort/rwsort --pmap-file=../../tests/ip-map.pmap --fields=src-service-host | ./rwgroup --pmap-file=../../tests/ip-map.pmap --id-fields=src-service-host | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwfilter = check_silk_app('rwfilter');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwsort --pmap-file=$file{ip_map} --fields=src-service-host | $rwgroup --pmap-file=$file{ip_map} --id-fields=src-service-host | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "44efab1abb941a646df2bbfe4d2b0550";

check_md5_output($md5, $cmd);
