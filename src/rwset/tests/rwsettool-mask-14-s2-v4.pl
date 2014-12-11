#! /usr/bin/perl -w
# MD5: 2390aeeb711bc2d5568e69e2130f9c84
# TEST: ./rwsettool --mask=14 ../../tests/set2-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --mask=14 $file{v4set2} | $rwsetcat";
my $md5 = "2390aeeb711bc2d5568e69e2130f9c84";

check_md5_output($md5, $cmd);
