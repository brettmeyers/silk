#! /usr/bin/perl -w
# MD5: c79677b9993f9510ea703d452c4c2764
# TEST: ./rwsetcat --count-ips --print-filename=0 ../../tests/set1-v4.set ../../tests/set2-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsetcat --count-ips --print-filename=0 $file{v4set1} $file{v4set2}";
my $md5 = "c79677b9993f9510ea703d452c4c2764";

check_md5_output($md5, $cmd);
