#! /usr/bin/perl -w
# MD5: e937eb6454a1673d4a5f2d0bc54dc091
# TEST: ./rwbag --sport-bytes=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal --mincounter=2000

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $rwbag = check_silk_app('rwbag');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-bytes=stdout $file{data} | $rwbagcat --key-format=decimal --mincounter=2000";
my $md5 = "e937eb6454a1673d4a5f2d0bc54dc091";

check_md5_output($md5, $cmd);
