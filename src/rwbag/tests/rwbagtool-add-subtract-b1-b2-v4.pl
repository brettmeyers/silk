#! /usr/bin/perl -w
# MD5: d41d8cd98f00b204e9800998ecf8427e
# TEST: ./rwbagtool --add ../../tests/bag1-v4.bag ../../tests/bag2-v4.bag | ./rwbagtool --subtract - ../../tests/bag1-v4.bag ../../tests/bag2-v4.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v4bag1} = get_data_or_exit77('v4bag1');
$file{v4bag2} = get_data_or_exit77('v4bag2');
my $cmd = "$rwbagtool --add $file{v4bag1} $file{v4bag2} | $rwbagtool --subtract - $file{v4bag1} $file{v4bag2} | $rwbagcat";
my $md5 = "d41d8cd98f00b204e9800998ecf8427e";

check_md5_output($md5, $cmd);
