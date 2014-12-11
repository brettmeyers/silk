#! /usr/bin/perl -w
# MD5: 3eea5a63b1065ad5fa29944a38e7228b
# TEST: ./rwbagtool --maximize ../../tests/bag3-v4.bag ../../tests/bag1-v4.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v4bag1} = get_data_or_exit77('v4bag1');
$file{v4bag3} = get_data_or_exit77('v4bag3');
my $cmd = "$rwbagtool --maximize $file{v4bag3} $file{v4bag1} | $rwbagcat";
my $md5 = "3eea5a63b1065ad5fa29944a38e7228b";

check_md5_output($md5, $cmd);
