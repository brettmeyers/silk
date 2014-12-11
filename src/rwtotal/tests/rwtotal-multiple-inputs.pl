#! /usr/bin/perl -w
# MD5: df9eb1c770e990e0f60bb14095792254
# TEST: ./rwtotal --sport --skip-zero ../../tests/data.rwf ../../tests/empty.rwf ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwtotal --sport --skip-zero $file{data} $file{empty} $file{data}";
my $md5 = "df9eb1c770e990e0f60bb14095792254";

check_md5_output($md5, $cmd);
