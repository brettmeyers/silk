#! /usr/bin/perl -w
# MD5: c14c49eda9648de13d5fb2422f152409
# TEST: ./rwuniq --pmap-file=../../tests/ip-map-v6.pmap --fields=src-service-host --sort-output ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_ip_map} = get_data_or_exit77('v6_ip_map');
check_features(qw(ipv6));
my $cmd = "$rwuniq --pmap-file=$file{v6_ip_map} --fields=src-service-host --sort-output $file{v6data}";
my $md5 = "c14c49eda9648de13d5fb2422f152409";

check_md5_output($md5, $cmd);
