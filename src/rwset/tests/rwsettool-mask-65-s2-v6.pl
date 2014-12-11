#! /usr/bin/perl -w
# MD5: d3d44e4f506b8e9e1eaa884cdc288a96
# TEST: ./rwsettool --mask=65 ../../tests/set2-v6.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsettool --mask=65 $file{v6set2} | $rwsetcat";
my $md5 = "d3d44e4f506b8e9e1eaa884cdc288a96";

check_md5_output($md5, $cmd);
