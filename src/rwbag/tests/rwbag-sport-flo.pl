#! /usr/bin/perl -w
# MD5: 4b544143c91248252629ef2e6acba1c3
# TEST: ./rwbag --sport-flow=stdout ../../tests/data.rwf | ./rwbagcat --key-format=decimal --column-separator=,

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-flow=stdout $file{data} | $rwbagcat --key-format=decimal --column-separator=,";
my $md5 = "4b544143c91248252629ef2e6acba1c3";

check_md5_output($md5, $cmd);
