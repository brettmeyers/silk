#! /usr/bin/perl -w
# MD5: c2250494883064b052cabaa25f472ea1
# TEST: ./rwuniq --fields=sport --packets=20 --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=sport --packets=20 --sort-output $file{data}";
my $md5 = "c2250494883064b052cabaa25f472ea1";

check_md5_output($md5, $cmd);
