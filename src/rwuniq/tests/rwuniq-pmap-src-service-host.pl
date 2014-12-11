#! /usr/bin/perl -w
# MD5: c14c49eda9648de13d5fb2422f152409
# TEST: ./rwuniq --pmap-file=../../tests/ip-map.pmap --fields=src-service-host --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{ip_map} = get_data_or_exit77('ip_map');
my $cmd = "$rwuniq --pmap-file=$file{ip_map} --fields=src-service-host --sort-output $file{data}";
my $md5 = "c14c49eda9648de13d5fb2422f152409";

check_md5_output($md5, $cmd);
