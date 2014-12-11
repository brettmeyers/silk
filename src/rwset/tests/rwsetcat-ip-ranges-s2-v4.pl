#! /usr/bin/perl -w
# MD5: 5114fa12e42a3f0adfa19da639e432e5
# TEST: ./rwsetcat --ip-ranges ../../tests/set2-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsetcat --ip-ranges $file{v4set2}";
my $md5 = "5114fa12e42a3f0adfa19da639e432e5";

check_md5_output($md5, $cmd);
