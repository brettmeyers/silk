#! /usr/bin/perl -w
# MD5: ae34ec298bda8b68b10b57bb12cf0739
# TEST: ./rwfilter --not-any-address=192.168.255,192-254.x --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --not-any-address=192.168.255,192-254.x --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "ae34ec298bda8b68b10b57bb12cf0739";

check_md5_output($md5, $cmd);
