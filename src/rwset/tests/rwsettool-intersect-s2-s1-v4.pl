#! /usr/bin/perl -w
# MD5: 0da1d94b36585742c1144cb266e29b37
# TEST: ./rwsettool --intersect ../../tests/set2-v4.set ../../tests/set1-v4.set | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --intersect $file{v4set2} $file{v4set1} | $rwsetcat --cidr";
my $md5 = "0da1d94b36585742c1144cb266e29b37";

check_md5_output($md5, $cmd);
