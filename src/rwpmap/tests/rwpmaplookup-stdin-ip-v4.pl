#! /usr/bin/perl -w
# MD5: 1f33f4c9c75ccd89c3654c308eb284da
# TEST: echo 192.168.72.72 | ./rwpmaplookup --map-file=../../tests/ip-map.pmap --ip-format=decimal --fields=key,value,input

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "echo 192.168.72.72 | $rwpmaplookup --map-file=$file{ip_map} --ip-format=decimal --fields=key,value,input";
my $md5 = "1f33f4c9c75ccd89c3654c308eb284da";

check_md5_output($md5, $cmd);
