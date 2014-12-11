#! /usr/bin/perl -w
# MD5: a8e4b0d6eabd6eaf16f8729da38b5541
# TEST: ./rwuniq --fields=stime --bin-time=3600 --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=stime --bin-time=3600 --sort-output $file{data}";
my $md5 = "a8e4b0d6eabd6eaf16f8729da38b5541";

check_md5_output($md5, $cmd);
