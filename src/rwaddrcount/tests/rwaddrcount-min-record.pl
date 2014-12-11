#! /usr/bin/perl -w
# MD5: e528b3689c659bbed36940c8c74455a9
# TEST: ./rwaddrcount --print-ips --sort-ips --ip-format=zero-padded --min-record=10 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --print-ips --sort-ips --ip-format=zero-padded --min-record=10 $file{data}";
my $md5 = "e528b3689c659bbed36940c8c74455a9";

check_md5_output($md5, $cmd);
