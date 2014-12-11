#! /usr/bin/perl -w
# MD5: 171325c6079fb681093f1fe57166d51b
# TEST: ./rwbagtool --divide ../../tests/bag1-v4.bag ../../tests/bag3-v4.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v4bag1} = get_data_or_exit77('v4bag1');
$file{v4bag3} = get_data_or_exit77('v4bag3');
my $cmd = "$rwbagtool --divide $file{v4bag1} $file{v4bag3} | $rwbagcat";
my $md5 = "171325c6079fb681093f1fe57166d51b";

check_md5_output($md5, $cmd);
