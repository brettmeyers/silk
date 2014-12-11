#! /usr/bin/perl -w
# MD5: b4bb6a0e5cd63d12460abca75637329a
# TEST: ./rwsettool --mask=26 ../../tests/set1-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsettool --mask=26 $file{v4set1} | $rwsetcat";
my $md5 = "b4bb6a0e5cd63d12460abca75637329a";

check_md5_output($md5, $cmd);
