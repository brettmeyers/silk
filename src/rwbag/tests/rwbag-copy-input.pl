#! /usr/bin/perl -w
# MD5: 06898de2a61b8470ffb9267e5231e19a
# TEST: ./rwbag --sport-flows=/dev/null --copy-input=stdout ../../tests/data.rwf | ./rwbag --sport-flows=- | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-flows=/dev/null --copy-input=stdout $file{data} | $rwbag --sport-flows=- | $rwbagcat --key-format=decimal";
my $md5 = "06898de2a61b8470ffb9267e5231e19a";

check_md5_output($md5, $cmd);
