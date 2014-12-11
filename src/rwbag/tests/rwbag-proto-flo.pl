#! /usr/bin/perl -w
# MD5: bd99d454bc9c54a6058eacc5c4f19845
# TEST: ./rwbag --proto-flow=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal --minkey=1

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --proto-flow=stdout $file{data} | $rwbagcat --key-format=decimal --minkey=1";
my $md5 = "bd99d454bc9c54a6058eacc5c4f19845";

check_md5_output($md5, $cmd);
