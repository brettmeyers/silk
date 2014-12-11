#! /usr/bin/perl -w
# MD5: c20f744faa9e31aa21b60d8225d8e3f2
# TEST: ./rwpackchecker --print-all ../../tests/data.rwf ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwpackchecker = check_silk_app('rwpackchecker');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwpackchecker --print-all $file{data} $file{empty}";
my $md5 = "c20f744faa9e31aa21b60d8225d8e3f2";

check_md5_output($md5, $cmd);
