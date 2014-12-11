#! /usr/bin/perl -w
# MD5: 2390aeeb711bc2d5568e69e2130f9c84
# TEST: ./rwsettool --mask=12 ../../tests/set1-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsettool --mask=12 $file{v4set1} | $rwsetcat";
my $md5 = "2390aeeb711bc2d5568e69e2130f9c84";

check_md5_output($md5, $cmd);
