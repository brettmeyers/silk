#! /usr/bin/perl -w
# MD5: 4359a8ef4ce4e58fe7dff9dfb3cf37eb
# TEST: ./rwsettool --mask=28 ../../tests/set1-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsettool --mask=28 $file{v4set1} | $rwsetcat";
my $md5 = "4359a8ef4ce4e58fe7dff9dfb3cf37eb";

check_md5_output($md5, $cmd);
