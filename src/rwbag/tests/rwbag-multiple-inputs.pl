#! /usr/bin/perl -w
# MD5: 124cf19c2ca056abc26494ec2851533d
# TEST: ./rwbag --sport-flow=stdout ../../tests/data.rwf ../../tests/empty.rwf ../../tests/data.rwf | ./rwbagcat --key-format=decimal

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwbag --sport-flow=stdout $file{data} $file{empty} $file{data} | $rwbagcat --key-format=decimal";
my $md5 = "124cf19c2ca056abc26494ec2851533d";

check_md5_output($md5, $cmd);
