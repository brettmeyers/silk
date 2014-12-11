#! /usr/bin/perl -w
# ERR_MD5: 3df5265608274668f88ad1d61251e284
# TEST: ../rwfilter/rwfilter --proto=0- --max-pass=10000 --pass=- ../../tests/data.rwf | ./rwcompare ../../tests/data.rwf - 2>&1

use strict;
use SiLKTests;

my $rwcompare = check_silk_app('rwcompare');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --proto=0- --max-pass=10000 --pass=- $file{data} | $rwcompare $file{data} - 2>&1";
my $md5 = "3df5265608274668f88ad1d61251e284";

check_md5_output($md5, $cmd, 1);
