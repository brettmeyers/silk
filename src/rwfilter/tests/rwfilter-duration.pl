#! /usr/bin/perl -w
# MD5: 9f45070e7410c1e3121ebccec54853b2
# TEST: ./rwfilter --duration=1-5 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --duration=1-5 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "9f45070e7410c1e3121ebccec54853b2";

check_md5_output($md5, $cmd);
