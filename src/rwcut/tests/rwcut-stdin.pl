#! /usr/bin/perl -w
# MD5: 63ee7b862dc8ddd94155b4687d83ed0f
# TEST: cat ../../tests/data.rwf | ./rwcut --fields=3-8

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "cat $file{data} | $rwcut --fields=3-8";
my $md5 = "63ee7b862dc8ddd94155b4687d83ed0f";

check_md5_output($md5, $cmd);
