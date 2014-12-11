#! /usr/bin/perl -w
# MD5: 8e4ae981669dd04d23ce93630345bfff
# TEST: cat ../../tests/data.rwf | ./rwuniq --fields=sport --sort-output

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "cat $file{data} | $rwuniq --fields=sport --sort-output";
my $md5 = "8e4ae981669dd04d23ce93630345bfff";

check_md5_output($md5, $cmd);
