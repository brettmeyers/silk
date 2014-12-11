#! /usr/bin/perl -w
# MD5: ae34ec298bda8b68b10b57bb12cf0739
# TEST: echo 192.168.192-255.x | ../rwset/rwsetbuild - - | ./rwfilter --anyset=- --fail=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my $rwsetbuild = check_silk_app('rwsetbuild');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "echo 192.168.192-255.x | $rwsetbuild - - | $rwfilter --anyset=- --fail=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "ae34ec298bda8b68b10b57bb12cf0739";

check_md5_output($md5, $cmd);
