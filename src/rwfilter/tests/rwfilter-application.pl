#! /usr/bin/perl -w
# MD5: 8ab8395fe4f59d74db29f526dd54a35c
# TEST: ./rwfilter --application=80 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --application=80 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "8ab8395fe4f59d74db29f526dd54a35c";

check_md5_output($md5, $cmd);
