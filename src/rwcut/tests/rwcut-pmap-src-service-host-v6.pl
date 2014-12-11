#! /usr/bin/perl -w
# MD5: 38e5516d30963f9ff6ad7a5f5043aa88
# TEST: ./rwcut --pmap-file=../../tests/ip-map-v6.pmap --fields=src-service-host ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwcut --pmap-file=$file{v6_ip_map} --fields=src-service-host $file{v6data}";
my $md5 = "38e5516d30963f9ff6ad7a5f5043aa88";

check_md5_output($md5, $cmd);
