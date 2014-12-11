#! /usr/bin/perl -w
# MD5: 62a27fef58ad7c9e94d056c01119ca61
# TEST: ./rwsettool --mask=52 ../../tests/set2-v6.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsettool --mask=52 $file{v6set2} | $rwsetcat";
my $md5 = "62a27fef58ad7c9e94d056c01119ca61";

check_md5_output($md5, $cmd);
