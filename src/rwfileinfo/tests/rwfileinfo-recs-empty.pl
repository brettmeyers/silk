#! /usr/bin/perl -w
# MD5: 897316929176464ebc9ad085f31e7284
# TEST: ./rwfileinfo --fields=7 --no-title ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwfileinfo = check_silk_app('rwfileinfo');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwfileinfo --fields=7 --no-title $file{empty}";
my $md5 = "897316929176464ebc9ad085f31e7284";

check_md5_output($md5, $cmd);
