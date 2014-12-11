#! /usr/bin/perl -w
# MD5: 9fc2ba85883ad3a9f274de56ab17e4a8
# TEST: ./rwsettool --mask=23 ../../tests/set1-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsettool --mask=23 $file{v4set1} | $rwsetcat";
my $md5 = "9fc2ba85883ad3a9f274de56ab17e4a8";

check_md5_output($md5, $cmd);
