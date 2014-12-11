#! /usr/bin/perl -w
# MD5: 1145be9f40cc1de0d6539f37c0a9a07a
# TEST: ./rwaddrcount --print-ips --sort-ips --no-title ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --print-ips --sort-ips --no-title $file{data}";
my $md5 = "1145be9f40cc1de0d6539f37c0a9a07a";

check_md5_output($md5, $cmd);
