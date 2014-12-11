#! /usr/bin/perl -w
# MD5: 50f0d33849838ee98aabf27e00b0472f
# TEST: ./rwcut --fields=8,initialFlags,sessionFlags ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=8,initialFlags,sessionFlags $file{data}";
my $md5 = "50f0d33849838ee98aabf27e00b0472f";

check_md5_output($md5, $cmd);
