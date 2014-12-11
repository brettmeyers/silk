#! /usr/bin/perl -w
# MD5: 9201dc199417ff97c881cfb3b2e4acb0
# TEST: ./rwbag --dip-bytes=stdout ../../tests/data.rwf | ./rwbagcat

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --dip-bytes=stdout $file{data} | $rwbagcat";
my $md5 = "9201dc199417ff97c881cfb3b2e4acb0";

check_md5_output($md5, $cmd);
