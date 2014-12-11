#! /usr/bin/perl -w
# MD5: c254c742204a7af52e8ae9d0a916db78
# TEST: ./rwcut --pmap-file=servhost:../../tests/ip-map-v6.pmap --fields=dst-servhost ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwcut --pmap-file=servhost:$file{v6_ip_map} --fields=dst-servhost $file{v6data}";
my $md5 = "c254c742204a7af52e8ae9d0a916db78";

check_md5_output($md5, $cmd);
