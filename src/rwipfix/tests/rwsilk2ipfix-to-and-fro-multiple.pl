#! /usr/bin/perl -w
# MD5: 393789257810fde6263977f90d106343
# TEST: ./rwsilk2ipfix ../../tests/empty.rwf ../../tests/data.rwf ../../tests/empty.rwf | ./rwipfix2silk | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwsilk2ipfix = check_silk_app('rwsilk2ipfix');
my $rwipfix2silk = check_silk_app('rwipfix2silk');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{empty} = get_data_or_exit77('empty');
$file{data} = get_data_or_exit77('data');
check_features(qw(ipfix));
my $cmd = "$rwsilk2ipfix $file{empty} $file{data} $file{empty} | $rwipfix2silk | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "393789257810fde6263977f90d106343";

check_md5_output($md5, $cmd);
