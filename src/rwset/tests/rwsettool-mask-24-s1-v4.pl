#! /usr/bin/perl -w
# MD5: e851c988d51ae0c6bfd622ed9f6d1612
# TEST: ./rwsettool --mask=24 ../../tests/set1-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsettool --mask=24 $file{v4set1} | $rwsetcat";
my $md5 = "e851c988d51ae0c6bfd622ed9f6d1612";

check_md5_output($md5, $cmd);
