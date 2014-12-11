#! /usr/bin/perl -w
# MD5: 06898de2a61b8470ffb9267e5231e19a
# TEST: cat ../../tests/data.rwf | ./rwbag --sport-flows=stdout | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "cat $file{data} | $rwbag --sport-flows=stdout | $rwbagcat --key-format=decimal";
my $md5 = "06898de2a61b8470ffb9267e5231e19a";

check_md5_output($md5, $cmd);
