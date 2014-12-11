#! /usr/bin/perl -w
# MD5: e3a8511346c06793545f8b7b9b783354
# TEST: ./rwtotal --sport --max-byte=2000 --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sport --max-byte=2000 --skip-zero $file{data}";
my $md5 = "e3a8511346c06793545f8b7b9b783354";

check_md5_output($md5, $cmd);
