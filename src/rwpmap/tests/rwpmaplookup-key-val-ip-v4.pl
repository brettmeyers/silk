#! /usr/bin/perl -w
# MD5: f3047174c7b81dcf91e237199a265767
# TEST: ./rwpmaplookup --map-file=../../tests/ip-map.pmap --no-title --no-files 192.168.72.72

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwpmaplookup --map-file=$file{ip_map} --no-title --no-files 192.168.72.72";
my $md5 = "f3047174c7b81dcf91e237199a265767";

check_md5_output($md5, $cmd);
