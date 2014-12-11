#! /usr/bin/perl -w
# MD5: 5360ad5a52678d4936e5a83822e86b1a
# TEST: ./rwfilter --proto=17 --print-volume-statistics=stdout ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwfilter --proto=17 --print-volume-statistics=stdout $file{v6data}";
my $md5 = "5360ad5a52678d4936e5a83822e86b1a";

check_md5_output($md5, $cmd);
