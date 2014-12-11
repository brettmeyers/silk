#! /usr/bin/perl -w
# MD5: 2ed8a84db65a7ca841fbf134447f2108
# TEST: ./rwfilter --any-cidr=2001:db8:c0:a8::c0:0/107,2001:db8:c0:a8::e0:0/107 --fail=stdout ../../tests/data-v6.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwfilter --any-cidr=2001:db8:c0:a8::c0:0/107,2001:db8:c0:a8::e0:0/107 --fail=stdout $file{v6data} | $rwcat --compression-method=none --byte-order=little";
my $md5 = "2ed8a84db65a7ca841fbf134447f2108";

check_md5_output($md5, $cmd);
