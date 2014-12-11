#! /usr/bin/perl -w
# MD5: 939c2e51a83c608154793eda074f55e7
# TEST: ./rwaddrcount --print-rec --sort-ips --ip-format=decimal --min-byte=2000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --print-rec --sort-ips --ip-format=decimal --min-byte=2000 $file{data}";
my $md5 = "939c2e51a83c608154793eda074f55e7";

check_md5_output($md5, $cmd);
