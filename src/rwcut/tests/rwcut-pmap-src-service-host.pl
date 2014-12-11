#! /usr/bin/perl -w
# MD5: 38e5516d30963f9ff6ad7a5f5043aa88
# TEST: ./rwcut --pmap-file=../../tests/ip-map.pmap --fields=src-service-host ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwcut --pmap-file=$file{ip_map} --fields=src-service-host $file{data}";
my $md5 = "38e5516d30963f9ff6ad7a5f5043aa88";

check_md5_output($md5, $cmd);
