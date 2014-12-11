#! /usr/bin/perl -w
# MD5: 4fb3d7f0698a85c2f547f2013aa3b3ae
# TEST: ./rwsettool --mask=63 ../../tests/set2-v6.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsettool --mask=63 $file{v6set2} | $rwsetcat";
my $md5 = "4fb3d7f0698a85c2f547f2013aa3b3ae";

check_md5_output($md5, $cmd);
