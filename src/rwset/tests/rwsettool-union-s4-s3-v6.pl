#! /usr/bin/perl -w
# MD5: 8145f9b279c72ee07e5839dd5977d60a
# TEST: ./rwsettool --union ../../tests/set4-v6.set ../../tests/set3-v6.set | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set3} = get_data_or_exit77('v6set3');
$file{v6set4} = get_data_or_exit77('v6set4');
my $cmd = "$rwsettool --union $file{v6set4} $file{v6set3} | $rwsetcat --cidr";
my $md5 = "8145f9b279c72ee07e5839dd5977d60a";

check_md5_output($md5, $cmd);
