#! /usr/bin/perl -w
# MD5: f08f8c2f07b8341cc9a1701351f4e0b2
# TEST: ./rwsettool --difference ../../tests/set2-v4.set ../../tests/set1-v4.set | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --difference $file{v4set2} $file{v4set1} | $rwsetcat --cidr";
my $md5 = "f08f8c2f07b8341cc9a1701351f4e0b2";

check_md5_output($md5, $cmd);
