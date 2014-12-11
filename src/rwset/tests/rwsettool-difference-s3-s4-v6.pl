#! /usr/bin/perl -w
# MD5: 0d25ddb9535f66ac44968d3c1eea25e7
# TEST: ./rwsettool --difference ../../tests/set3-v6.set ../../tests/set4-v6.set | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set3} = get_data_or_exit77('v6set3');
$file{v6set4} = get_data_or_exit77('v6set4');
my $cmd = "$rwsettool --difference $file{v6set3} $file{v6set4} | $rwsetcat --cidr";
my $md5 = "0d25ddb9535f66ac44968d3c1eea25e7";

check_md5_output($md5, $cmd);
