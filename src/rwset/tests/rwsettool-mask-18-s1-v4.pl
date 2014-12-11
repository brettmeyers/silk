#! /usr/bin/perl -w
# MD5: 11c396c1b1d1f875da4ab502134ed5f0
# TEST: ./rwsettool --mask=18 ../../tests/set1-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsettool --mask=18 $file{v4set1} | $rwsetcat";
my $md5 = "11c396c1b1d1f875da4ab502134ed5f0";

check_md5_output($md5, $cmd);
