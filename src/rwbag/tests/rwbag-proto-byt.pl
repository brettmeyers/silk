#! /usr/bin/perl -w
# MD5: f94064f4f99d8061d01be37e7e97ae12
# TEST: ./rwbag --proto-bytes=- ../../tests/data.rwf | ./rwbagcat --key-format=decimal --minkey=1 --maxkey=20 --zero-counts

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --proto-bytes=- $file{data} | $rwbagcat --key-format=decimal --minkey=1 --maxkey=20 --zero-counts";
my $md5 = "f94064f4f99d8061d01be37e7e97ae12";

check_md5_output($md5, $cmd);
