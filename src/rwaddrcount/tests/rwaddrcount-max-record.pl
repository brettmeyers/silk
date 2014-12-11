#! /usr/bin/perl -w
# MD5: 4f24a14c6186472107c586446693bf28
# TEST: ./rwaddrcount --print-ips --sort-ips --ip-format=zero-padded --max-record=10 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --print-ips --sort-ips --ip-format=zero-padded --max-record=10 $file{data}";
my $md5 = "4f24a14c6186472107c586446693bf28";

check_md5_output($md5, $cmd);
