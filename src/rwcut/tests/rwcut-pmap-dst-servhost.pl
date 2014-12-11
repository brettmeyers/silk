#! /usr/bin/perl -w
# MD5: c254c742204a7af52e8ae9d0a916db78
# TEST: ./rwcut --pmap-file=servhost:../../tests/ip-map.pmap --fields=dst-servhost ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwcut --pmap-file=servhost:$file{ip_map} --fields=dst-servhost $file{data}";
my $md5 = "c254c742204a7af52e8ae9d0a916db78";

check_md5_output($md5, $cmd);
