#! /usr/bin/perl -w
# MD5: 4dafa4a62ff11616bab4f87f031d05c0
# TEST: ./rwfilter --packets=1-50 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --packets=1-50 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "4dafa4a62ff11616bab4f87f031d05c0";

check_md5_output($md5, $cmd);
