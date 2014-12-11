#! /usr/bin/perl -w
# MD5: d2376b718a53f35ade9b08017e76bb2a
# TEST: ./rwsettool --mask=61 ../../tests/set2-v6.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsettool --mask=61 $file{v6set2} | $rwsetcat";
my $md5 = "d2376b718a53f35ade9b08017e76bb2a";

check_md5_output($md5, $cmd);
