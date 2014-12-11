#! /usr/bin/perl -w
# MD5: ddf07c0f0fe7da1d6becefce6202d43c
# TEST: ./rwsort --fields=bytes ../../tests/data.rwf ../../tests/empty.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwsort --fields=bytes $file{data} $file{empty} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "ddf07c0f0fe7da1d6becefce6202d43c";

check_md5_output($md5, $cmd);
