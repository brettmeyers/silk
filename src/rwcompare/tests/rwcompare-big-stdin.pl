#! /usr/bin/perl -w
# MD5: d41d8cd98f00b204e9800998ecf8427e
# TEST: ../rwcat/rwcat --byte-order=big ../../tests/data.rwf | ./rwcompare ../../tests/data.rwf -

use strict;
use SiLKTests;

my $rwcompare = check_silk_app('rwcompare');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcat --byte-order=big $file{data} | $rwcompare $file{data} -";
my $md5 = "d41d8cd98f00b204e9800998ecf8427e";

check_md5_output($md5, $cmd);
