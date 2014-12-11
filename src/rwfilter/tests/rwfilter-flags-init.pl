#! /usr/bin/perl -w
# MD5: 7df8deef7c98f0797e3afd1279fc6742
# TEST: ./rwfilter --flags-init=S/SA --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --flags-init=S/SA --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "7df8deef7c98f0797e3afd1279fc6742";

check_md5_output($md5, $cmd);
