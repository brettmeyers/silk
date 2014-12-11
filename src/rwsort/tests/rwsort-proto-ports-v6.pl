#! /usr/bin/perl -w
# MD5: b30c71f0658492d4943f02cc84993b58
# TEST: ./rwsort --fields=5,3-4 ../../tests/data-v6.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwsort --fields=5,3-4 $file{v6data} | $rwcat --compression-method=none --byte-order=little";
my $md5 = "b30c71f0658492d4943f02cc84993b58";

check_md5_output($md5, $cmd);
