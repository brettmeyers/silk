#! /usr/bin/perl -w
# MD5: 424c1b7ecd47f25e8824cba126e34376
# TEST: ./rwsettool --mask=30 ../../tests/set2-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --mask=30 $file{v4set2} | $rwsetcat";
my $md5 = "424c1b7ecd47f25e8824cba126e34376";

check_md5_output($md5, $cmd);
