#! /usr/bin/perl -w
# MD5: 172fc9fb19814497b4de0cfea626fbbf
# TEST: ./rwsettool --mask=25 ../../tests/set2-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --mask=25 $file{v4set2} | $rwsetcat";
my $md5 = "172fc9fb19814497b4de0cfea626fbbf";

check_md5_output($md5, $cmd);
