#! /usr/bin/perl -w
# MD5: d57a7ce057f42c4585e28acc4dc4a895
# TEST: ./rwpmaplookup --map-file=../../tests/ip-map.pmap --no-title --fields=block,key,value --no-files 172.16.17.18 172.30.31.32

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmaplookup --map-file=$file{ip_map} --no-title --fields=block,key,value --no-files 172.16.17.18 172.30.31.32";
my $md5 = "d57a7ce057f42c4585e28acc4dc4a895";

check_md5_output($md5, $cmd);
