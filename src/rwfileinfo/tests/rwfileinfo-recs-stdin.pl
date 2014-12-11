#! /usr/bin/perl -w
# MD5: 5268dc9c3b85e8a8b3fba1e2bab9e52c
# TEST: cat ../../tests/data.rwf | ./rwfileinfo --fields=count-records -

use strict;
use SiLKTests;

my $rwfileinfo = check_silk_app('rwfileinfo');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "cat $file{data} | $rwfileinfo --fields=count-records -";
my $md5 = "5268dc9c3b85e8a8b3fba1e2bab9e52c";

check_md5_output($md5, $cmd);
