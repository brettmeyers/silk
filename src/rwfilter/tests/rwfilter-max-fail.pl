#! /usr/bin/perl -w
# MD5: ac21ed686b9152dbd03df202de740c8f
# TEST: ./rwfilter --proto=17 --max-fail=200 --fail=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --proto=17 --max-fail=200 --fail=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "ac21ed686b9152dbd03df202de740c8f";

check_md5_output($md5, $cmd);
