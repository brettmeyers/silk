#! /usr/bin/perl -w
# MD5: 9793376a6cb2a38af77cd9298bebe25f
# TEST: ./rwsettool --mask=29 ../../tests/set1-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsettool --mask=29 $file{v4set1} | $rwsetcat";
my $md5 = "9793376a6cb2a38af77cd9298bebe25f";

check_md5_output($md5, $cmd);
