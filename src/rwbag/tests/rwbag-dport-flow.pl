#! /usr/bin/perl -w
# MD5: ed5f39ee863b1077932c845374075b34
# TEST: ./rwbag --dport-flow=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --dport-flow=stdout $file{data} | $rwbagcat --key-format=decimal";
my $md5 = "ed5f39ee863b1077932c845374075b34";

check_md5_output($md5, $cmd);
