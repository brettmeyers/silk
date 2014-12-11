#! /usr/bin/perl -w
# MD5: 8e92462b2264f0f3e03a961a2c053015
# TEST: ./rwsettool --mask=27 ../../tests/set2-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --mask=27 $file{v4set2} | $rwsetcat";
my $md5 = "8e92462b2264f0f3e03a961a2c053015";

check_md5_output($md5, $cmd);
