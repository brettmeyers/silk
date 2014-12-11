#! /usr/bin/perl -w
# MD5: 9cfba99f5bef06cc0b32fc1efca8ec7b
# TEST: ./rwsettool --mask=67 ../../tests/set1-v6.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set1} = get_data_or_exit77('v6set1');
check_features(qw(ipset_v6));
my $cmd = "$rwsettool --mask=67 $file{v6set1} | $rwsetcat";
my $md5 = "9cfba99f5bef06cc0b32fc1efca8ec7b";

check_md5_output($md5, $cmd);
