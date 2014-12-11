#! /usr/bin/perl -w
# MD5: e1cdefa5f9d18f8d695822e677a05efb
# TEST: ./rwsettool --difference ../../tests/set1-v4.set ../../tests/set2-v4.set | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --difference $file{v4set1} $file{v4set2} | $rwsetcat --cidr";
my $md5 = "e1cdefa5f9d18f8d695822e677a05efb";

check_md5_output($md5, $cmd);
