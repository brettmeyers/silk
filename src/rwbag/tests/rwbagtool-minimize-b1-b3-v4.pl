#! /usr/bin/perl -w
# MD5: c0647de1d0392b54d606a89337856769
# TEST: ./rwbagtool --minimize ../../tests/bag1-v4.bag ../../tests/bag3-v4.bag | ./rwbagcat

use strict;
use SiLKTests;

my $rwbagtool = check_silk_app('rwbagtool');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{v4bag1} = get_data_or_exit77('v4bag1');
$file{v4bag3} = get_data_or_exit77('v4bag3');
my $cmd = "$rwbagtool --minimize $file{v4bag1} $file{v4bag3} | $rwbagcat";
my $md5 = "c0647de1d0392b54d606a89337856769";

check_md5_output($md5, $cmd);
