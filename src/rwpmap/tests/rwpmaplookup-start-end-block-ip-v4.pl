#! /usr/bin/perl -w
# MD5: ee6115e1b5f28bf8f53e13fd0ba01945
# TEST: ./rwpmaplookup --map-file=../../tests/ip-map.pmap --no-title --fields=start-block,end-block,value --no-files 172.16.17.18 172.30.31.32

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmaplookup --map-file=$file{ip_map} --no-title --fields=start-block,end-block,value --no-files 172.16.17.18 172.30.31.32";
my $md5 = "ee6115e1b5f28bf8f53e13fd0ba01945";

check_md5_output($md5, $cmd);
