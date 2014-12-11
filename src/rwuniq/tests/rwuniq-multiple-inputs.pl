#! /usr/bin/perl -w
# MD5: 8e4ae981669dd04d23ce93630345bfff
# TEST: ./rwuniq --fields=sport --sort-output ../../tests/empty.rwf ../../tests/data.rwf ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwuniq --fields=sport --sort-output $file{empty} $file{data} $file{empty}";
my $md5 = "8e4ae981669dd04d23ce93630345bfff";

check_md5_output($md5, $cmd);
