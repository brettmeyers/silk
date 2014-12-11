#! /usr/bin/perl -w
# MD5: a50cb8710475c0266a4212bd9088d375
# TEST: ./rwsettool --mask=58 ../../tests/set2-v6.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsettool --mask=58 $file{v6set2} | $rwsetcat";
my $md5 = "a50cb8710475c0266a4212bd9088d375";

check_md5_output($md5, $cmd);
