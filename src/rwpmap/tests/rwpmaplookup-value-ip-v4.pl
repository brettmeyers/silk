#! /usr/bin/perl -w
# MD5: 190cd06ff7f62478d2b233ed5117119c
# TEST: ./rwpmaplookup --map-file=../../tests/ip-map.pmap --fields=value --no-title -delim --no-files 192.168.72.72

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmaplookup --map-file=$file{ip_map} --fields=value --no-title -delim --no-files 192.168.72.72";
my $md5 = "190cd06ff7f62478d2b233ed5117119c";

check_md5_output($md5, $cmd);
