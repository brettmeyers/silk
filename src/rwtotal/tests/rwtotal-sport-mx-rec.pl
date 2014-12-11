#! /usr/bin/perl -w
# MD5: 8d5e6f762f8c2faaa503a1353cc56372
# TEST: ./rwtotal --sport --max-record=10 --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sport --max-record=10 --skip-zero $file{data}";
my $md5 = "8d5e6f762f8c2faaa503a1353cc56372";

check_md5_output($md5, $cmd);
