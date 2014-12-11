#! /usr/bin/perl -w
# MD5: 2fe9b77b2b66f7b38bcd0140066cfdd8
# TEST: ./rwcount --bin-size=3600 --load-scheme=0 --skip-zero --start-time=2009/02/11T20:30:00 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=3600 --load-scheme=0 --skip-zero --start-time=2009/02/11T20:30:00 $file{data}";
my $md5 = "2fe9b77b2b66f7b38bcd0140066cfdd8";

check_md5_output($md5, $cmd);
