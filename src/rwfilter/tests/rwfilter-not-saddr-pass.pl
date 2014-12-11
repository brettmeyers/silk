#! /usr/bin/perl -w
# MD5: 080a68380d4ece908e36cff7be9d02bb
# TEST: ./rwfilter --not-saddr=10.252-255.0-255.0,1-100,102-255,101 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --not-saddr=10.252-255.0-255.0,1-100,102-255,101 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "080a68380d4ece908e36cff7be9d02bb";

check_md5_output($md5, $cmd);
