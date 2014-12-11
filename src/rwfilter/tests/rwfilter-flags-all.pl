#! /usr/bin/perl -w
# MD5: 709c46a9c0096beb4ddba3ce04923780
# TEST: ./rwfilter --flags-all=R/R --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --flags-all=R/R --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "709c46a9c0096beb4ddba3ce04923780";

check_md5_output($md5, $cmd);
