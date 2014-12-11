#! /usr/bin/perl -w
# MD5: a71b97bd67b48826498d47817ebe67fb
# TEST: ./rwcut --fields=5,4,3 --no-final-delimiter ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=5,4,3 --no-final-delimiter $file{data}";
my $md5 = "a71b97bd67b48826498d47817ebe67fb";

check_md5_output($md5, $cmd);
