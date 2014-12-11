#! /usr/bin/perl -w
# MD5: eef2b3bd1bc3f58e8605b38fa4794ed2
# TEST: ./rwsettool --intersect ../../tests/set2-v4.set ../../tests/set1-v4.set  > /tmp/rwsettool-symmet-diff-s2-s1-v4-intersect  && ./rwsettool --union ../../tests/set2-v4.set ../../tests/set1-v4.set | ./rwsettool --difference - /tmp/rwsettool-symmet-diff-s2-s1-v4-intersect | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
$file{v4set2} = get_data_or_exit77('v4set2');
my %temp;
$temp{intersect} = make_tempname('intersect');
my $cmd = "$rwsettool --intersect $file{v4set2} $file{v4set1}  > $temp{intersect}  && $rwsettool --union $file{v4set2} $file{v4set1} | $rwsettool --difference - $temp{intersect} | $rwsetcat --cidr";
my $md5 = "eef2b3bd1bc3f58e8605b38fa4794ed2";

check_md5_output($md5, $cmd);
