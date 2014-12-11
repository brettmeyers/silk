#! /usr/bin/perl -w
# MD5: 4c7641dc5a6047054477e852ee98abd3
# TEST: ../rwcut/rwcut --fields=sip --no-title --num-rec=200 --delimited ../../tests/data-v6.rwf | ../rwset/rwsetbuild - /tmp/rwpmaplookup-ipset-ip-v6-file1 && ./rwpmaplookup --map-file=../../tests/ip-map-v6.pmap --ip-format=zero-padded --fields=key,value,input --ipset-files /tmp/rwpmaplookup-ipset-ip-v6-file1

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my $rwcut = check_silk_app('rwcut');
my $rwsetbuild = check_silk_app('rwsetbuild');
my %file;
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
$file{v6data} = get_data_or_exit77('v6data');
my %temp;
$temp{file1} = make_tempname('file1');
check_features(qw(ipv6));
my $cmd = "$rwcut --fields=sip --no-title --num-rec=200 --delimited $file{v6data} | $rwsetbuild - $temp{file1} && $rwpmaplookup --map-file=$file{v6_ip_map} --ip-format=zero-padded --fields=key,value,input --ipset-files $temp{file1}";
my $md5 = "4c7641dc5a6047054477e852ee98abd3";

check_md5_output($md5, $cmd);
