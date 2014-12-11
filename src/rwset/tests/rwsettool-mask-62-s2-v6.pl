#! /usr/bin/perl -w
# MD5: 77b0fa4a4d5e90e6b2c8795b1289af8d
# TEST: ./rwsettool --mask=62 ../../tests/set2-v6.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsettool --mask=62 $file{v6set2} | $rwsetcat";
my $md5 = "77b0fa4a4d5e90e6b2c8795b1289af8d";

check_md5_output($md5, $cmd);
