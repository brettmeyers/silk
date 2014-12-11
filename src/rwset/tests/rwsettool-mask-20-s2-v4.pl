#! /usr/bin/perl -w
# MD5: f869ed0e97e0d94dbd6cc76e84d22892
# TEST: ./rwsettool --mask=20 ../../tests/set2-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --mask=20 $file{v4set2} | $rwsetcat";
my $md5 = "f869ed0e97e0d94dbd6cc76e84d22892";

check_md5_output($md5, $cmd);
