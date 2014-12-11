#! /usr/bin/perl -w
# MD5: b6cf9b8197bab2d3306923e637eea9cd
# TEST: ./rwbag --dip-packets=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --dip-packets=stdout $file{data} | $rwbagcat --key-format=decimal";
my $md5 = "b6cf9b8197bab2d3306923e637eea9cd";

check_md5_output($md5, $cmd);
