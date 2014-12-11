#! /usr/bin/perl -w
# MD5: e92f0c93dd895f9d18bb55d1fd1e5f3c
# TEST: ./rwbagtool --scalar-multiply=2 ../../tests/bag1-v4.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v4bag1} = get_data_or_exit77('v4bag1');
my $cmd = "$rwbagtool --scalar-multiply=2 $file{v4bag1} | $rwbagcat";
my $md5 = "e92f0c93dd895f9d18bb55d1fd1e5f3c";

check_md5_output($md5, $cmd);
