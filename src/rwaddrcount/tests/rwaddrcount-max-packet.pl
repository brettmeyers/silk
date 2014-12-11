#! /usr/bin/perl -w
# MD5: dfdb0cf11a9f8dba1c3569856f7f2232
# TEST: ./rwaddrcount --use-dest --print-rec --sort-ips --max-packet=20 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwaddrcount --use-dest --print-rec --sort-ips --max-packet=20 $file{data}";
my $md5 = "dfdb0cf11a9f8dba1c3569856f7f2232";

check_md5_output($md5, $cmd);
