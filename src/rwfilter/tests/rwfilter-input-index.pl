#! /usr/bin/perl -w
# MD5: 8495cd0b8ab12bfb8e8b805d519ebaad
# TEST: ./rwfilter --input-index=10 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --input-index=10 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "8495cd0b8ab12bfb8e8b805d519ebaad";

check_md5_output($md5, $cmd);
