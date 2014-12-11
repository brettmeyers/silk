#! /usr/bin/perl -w
# MD5: fd4b4c2fe7eaf0eb498f524ec86bcd82
# TEST: ./rwsilk2ipfix ../../tests/data-v6.rwf | ./rwipfix2silk --silk-output=stdout | ../rwcat/rwcat --compression-method=none --byte-order=little

use strict;
use SiLKTests;

my $rwsilk2ipfix = check_silk_app('rwsilk2ipfix');
my $rwipfix2silk = check_silk_app('rwipfix2silk');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipfix ipv6));
my $cmd = "$rwsilk2ipfix $file{v6data} | $rwipfix2silk --silk-output=stdout | $rwcat --compression-method=none --byte-order=little";
my $md5 = "fd4b4c2fe7eaf0eb498f524ec86bcd82";

check_md5_output($md5, $cmd);
