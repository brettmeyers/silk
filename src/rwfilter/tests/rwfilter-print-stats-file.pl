#! /usr/bin/perl -w
# MD5: 52f4ec7de9b7047cdf0f112f66919edd
# TEST: ./rwfilter --proto=17 --print-statistics --print-filenames --pass=/dev/null ../../tests/data.rwf 2>&1

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --proto=17 --print-statistics --print-filenames --pass=/dev/null $file{data} 2>&1";
my $md5 = "52f4ec7de9b7047cdf0f112f66919edd";

check_md5_output($md5, $cmd);
