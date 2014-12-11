#! /usr/bin/perl -w
# MD5: cad85a4dd62379f2d816e83d546fbdcb
# TEST: ./rwfilter --aport=25 --proto=6 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --aport=25 --proto=6 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "cad85a4dd62379f2d816e83d546fbdcb";

check_md5_output($md5, $cmd);
