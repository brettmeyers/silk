#! /usr/bin/perl -w
# MD5: f1a89478a8a8a2193f278e0425136870
# TEST: ./rwsetmember --count 10.x.x.x ../../tests/set1-v4.set ../../tests/set2-v4.set | sed 's,.*/,,'

use strict;
use SiLKTests;

my $rwsetmember = check_silk_app('rwsetmember');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsetmember --count 10.x.x.x $file{v4set1} $file{v4set2} | sed 's,.*/,,'";
my $md5 = "f1a89478a8a8a2193f278e0425136870";

check_md5_output($md5, $cmd);
