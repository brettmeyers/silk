#! /usr/bin/perl -w
# MD5: 9b01d21e573b5da24d7583dc75a3e675
# TEST: ./rwsettool --mask=65 ../../tests/set1-v6.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set1} = get_data_or_exit77('v6set1');
check_features(qw(ipset_v6));
my $cmd = "$rwsettool --mask=65 $file{v6set1} | $rwsetcat";
my $md5 = "9b01d21e573b5da24d7583dc75a3e675";

check_md5_output($md5, $cmd);
