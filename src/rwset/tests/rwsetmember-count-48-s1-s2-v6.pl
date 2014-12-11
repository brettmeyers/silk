#! /usr/bin/perl -w
# MD5: a8e6aecc9f67fa3edab8c26641ecab70
# TEST: ./rwsetmember --count 2001:db8:0:x:x:x:x:x ../../tests/set1-v6.set ../../tests/set2-v6.set | sed 's,.*/,,'

use strict;
use SiLKTests;

my $rwsetmember = check_silk_app('rwsetmember');
my %file;
$file{v6set1} = get_data_or_exit77('v6set1');
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsetmember --count 2001:db8:0:x:x:x:x:x $file{v6set1} $file{v6set2} | sed 's,.*/,,'";
my $md5 = "a8e6aecc9f67fa3edab8c26641ecab70";

check_md5_output($md5, $cmd);
