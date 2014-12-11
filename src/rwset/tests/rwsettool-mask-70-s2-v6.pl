#! /usr/bin/perl -w
# MD5: a72eb3955ebad9a45c7e71536f7f3fed
# TEST: ./rwsettool --mask=70 ../../tests/set2-v6.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsettool --mask=70 $file{v6set2} | $rwsetcat";
my $md5 = "a72eb3955ebad9a45c7e71536f7f3fed";

check_md5_output($md5, $cmd);
