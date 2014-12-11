#! /usr/bin/perl -w
# MD5: 595af07378629efa04030d6096e1b929
# TEST: ./rwsetcat --ip-ranges --ip-format=zero-padded ../../tests/set1-v6.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set1} = get_data_or_exit77('v6set1');
check_features(qw(ipset_v6));
my $cmd = "$rwsetcat --ip-ranges --ip-format=zero-padded $file{v6set1}";
my $md5 = "595af07378629efa04030d6096e1b929";

check_md5_output($md5, $cmd);
