#! /usr/bin/perl -w
# MD5: c52632951ea930f61237abf63b440b77
# TEST: ./rwuniq --fields=9,11 --timestamp-format=default --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=9,11 --timestamp-format=default --sort-output $file{data}";
my $md5 = "c52632951ea930f61237abf63b440b77";

check_md5_output($md5, $cmd);
