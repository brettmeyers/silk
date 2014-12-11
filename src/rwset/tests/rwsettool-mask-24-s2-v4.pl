#! /usr/bin/perl -w
# MD5: 8646b473804285f3d5853e78a14f90bd
# TEST: ./rwsettool --mask=24 ../../tests/set2-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --mask=24 $file{v4set2} | $rwsetcat";
my $md5 = "8646b473804285f3d5853e78a14f90bd";

check_md5_output($md5, $cmd);
