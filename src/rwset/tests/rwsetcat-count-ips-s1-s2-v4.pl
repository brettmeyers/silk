#! /usr/bin/perl -w
# MD5: 087178237a37fcc17e25b00e815591cb
# TEST: ./rwsetcat --count-ips ../../tests/set1-v4.set ../../tests/set2-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsetcat --count-ips $file{v4set1} $file{v4set2}";
my $md5 = "087178237a37fcc17e25b00e815591cb";

check_md5_output($md5, $cmd);
