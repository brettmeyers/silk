#! /usr/bin/perl -w
# MD5: 393789257810fde6263977f90d106343
# TEST: ./rwfilter --ip-version=4 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --ip-version=4 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "393789257810fde6263977f90d106343";

check_md5_output($md5, $cmd);
