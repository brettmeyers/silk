#! /usr/bin/perl -w
# MD5: 080a68380d4ece908e36cff7be9d02bb
# TEST: echo 10.252-255.x.x | ../rwset/rwsetbuild - - | ./rwfilter --not-sipset=- --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my $rwsetbuild = check_silk_app('rwsetbuild');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "echo 10.252-255.x.x | $rwsetbuild - - | $rwfilter --not-sipset=- --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "080a68380d4ece908e36cff7be9d02bb";

check_md5_output($md5, $cmd);
