#! /usr/bin/perl -w
# MD5: f25abf26342727e932e35d9bbb1c614b
# TEST: ./rwaddrcount --print-rec --sort-ips --ip-format=decimal --max-byte=2000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --print-rec --sort-ips --ip-format=decimal --max-byte=2000 $file{data}";
my $md5 = "f25abf26342727e932e35d9bbb1c614b";

check_md5_output($md5, $cmd);
