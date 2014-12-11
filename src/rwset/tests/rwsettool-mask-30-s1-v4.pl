#! /usr/bin/perl -w
# MD5: 115bbc2ae4b54335ef810ac70bce47fb
# TEST: ./rwsettool --mask=30 ../../tests/set1-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsettool --mask=30 $file{v4set1} | $rwsetcat";
my $md5 = "115bbc2ae4b54335ef810ac70bce47fb";

check_md5_output($md5, $cmd);
