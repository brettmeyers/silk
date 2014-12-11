#! /usr/bin/perl -w
# MD5: 3fcdfd31f87c6d518f909dd9be47171f
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ./rwsort --pmap-file=servhost:../../tests/ip-map.pmap --fields=dst-servhost | ../rwuniq/rwuniq --pmap-file=servhost:../../tests/ip-map.pmap --fields=dst-servhost --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwfilter = check_silk_app('rwfilter');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwsort --pmap-file=servhost:$file{ip_map} --fields=dst-servhost | $rwuniq --pmap-file=servhost:$file{ip_map} --fields=dst-servhost --presorted-input";
my $md5 = "3fcdfd31f87c6d518f909dd9be47171f";

check_md5_output($md5, $cmd);
