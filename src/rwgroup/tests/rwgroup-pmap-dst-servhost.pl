#! /usr/bin/perl -w
# MD5: 68a6395b288dd1c6fc19144a55f9f47e
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ../rwsort/rwsort --pmap-file=servhost:../../tests/ip-map.pmap --fields=dst-servhost | ./rwgroup --pmap-file=servhost:../../tests/ip-map.pmap --id-fields=dst-servhost | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwfilter = check_silk_app('rwfilter');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwsort --pmap-file=servhost:$file{ip_map} --fields=dst-servhost | $rwgroup --pmap-file=servhost:$file{ip_map} --id-fields=dst-servhost | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "68a6395b288dd1c6fc19144a55f9f47e";

check_md5_output($md5, $cmd);
