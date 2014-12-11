#! /usr/bin/perl -w
# MD5: 2adae71d6fe8383839747da0ee34287d
# TEST: ../rwfilter/rwfilter --proto=1 --pass=- ../../tests/data.rwf | ./rwuniq --fields=icmpTypeCode --sort-output

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --proto=1 --pass=- $file{data} | $rwuniq --fields=icmpTypeCode --sort-output";
my $md5 = "2adae71d6fe8383839747da0ee34287d";

check_md5_output($md5, $cmd);
