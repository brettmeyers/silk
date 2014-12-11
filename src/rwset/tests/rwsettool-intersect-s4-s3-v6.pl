#! /usr/bin/perl -w
# MD5: cac0436a41fc954fff0357ceab3631cc
# TEST: ./rwsettool --intersect ../../tests/set4-v6.set ../../tests/set3-v6.set | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set3} = get_data_or_exit77('v6set3');
$file{v6set4} = get_data_or_exit77('v6set4');
my $cmd = "$rwsettool --intersect $file{v6set4} $file{v6set3} | $rwsetcat --cidr";
my $md5 = "cac0436a41fc954fff0357ceab3631cc";

check_md5_output($md5, $cmd);
