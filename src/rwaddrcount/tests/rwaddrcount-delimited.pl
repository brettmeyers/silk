#! /usr/bin/perl -w
# MD5: 1183b88699280bfa51b1891b46fcab50
# TEST: ./rwaddrcount --print-rec --sort-ips --delimited=, ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --print-rec --sort-ips --delimited=, $file{data}";
my $md5 = "1183b88699280bfa51b1891b46fcab50";

check_md5_output($md5, $cmd);
