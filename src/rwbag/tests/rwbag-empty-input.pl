#! /usr/bin/perl -w
# MD5: d41d8cd98f00b204e9800998ecf8427e
# TEST: ./rwbag --sport-flow=stdout ../../tests/empty.rwf | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwbag --sport-flow=stdout $file{empty} | $rwbagcat --key-format=decimal";
my $md5 = "d41d8cd98f00b204e9800998ecf8427e";

check_md5_output($md5, $cmd);
