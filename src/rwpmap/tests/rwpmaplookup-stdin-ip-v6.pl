#! /usr/bin/perl -w
# MD5: aaed9a8e1828b8c7d81a98d6f7f33860
# TEST: ../rwcut/rwcut --fields=sip --no-title --num-rec=200 --delimited ../../tests/data-v6.rwf | ./rwpmaplookup --map-file=../../tests/ip-map-v6.pmap --ip-format=zero-padded --fields=key,value,input

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my $rwcut = check_silk_app('rwcut');
my %file;
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwcut --fields=sip --no-title --num-rec=200 --delimited $file{v6data} | $rwpmaplookup --map-file=$file{v6_ip_map} --ip-format=zero-padded --fields=key,value,input";
my $md5 = "aaed9a8e1828b8c7d81a98d6f7f33860";

check_md5_output($md5, $cmd);
