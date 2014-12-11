#! /usr/bin/perl -w
# MD5: cb5acb1ff15011c2d92ef4d4d87cdedd
# TEST: ./rwsort --fields=1 ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=1 $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "cb5acb1ff15011c2d92ef4d4d87cdedd";

check_md5_output($md5, $cmd);
