#! /usr/bin/perl -w
# MD5: db580689b9b0844a0a066b7b4c252621
# TEST: ./rwsetmember --count 10.0.15.128/25 ../../tests/set1-v4.set ../../tests/set2-v4.set | sed 's,.*/,,'

use strict;
use SiLKTests;

my $rwsetmember = check_silk_app('rwsetmember');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsetmember --count 10.0.15.128/25 $file{v4set1} $file{v4set2} | sed 's,.*/,,'";
my $md5 = "db580689b9b0844a0a066b7b4c252621";

check_md5_output($md5, $cmd);
