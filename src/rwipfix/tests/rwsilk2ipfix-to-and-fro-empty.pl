#! /usr/bin/perl -w
# MD5: d72d949264f8735075fdc3315b08bfb2
# TEST: ./rwsilk2ipfix ../../tests/empty.rwf | ./rwipfix2silk -  | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwsilk2ipfix = check_silk_app('rwsilk2ipfix');
my $rwipfix2silk = check_silk_app('rwipfix2silk');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{empty} = get_data_or_exit77('empty');
check_features(qw(ipfix));
my $cmd = "$rwsilk2ipfix $file{empty} | $rwipfix2silk -  | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "d72d949264f8735075fdc3315b08bfb2";

check_md5_output($md5, $cmd);
