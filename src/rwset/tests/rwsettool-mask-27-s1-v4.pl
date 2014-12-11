#! /usr/bin/perl -w
# MD5: 25d0d056b011228ff3b3a1de91a757d3
# TEST: ./rwsettool --mask=27 ../../tests/set1-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsettool --mask=27 $file{v4set1} | $rwsetcat";
my $md5 = "25d0d056b011228ff3b3a1de91a757d3";

check_md5_output($md5, $cmd);
