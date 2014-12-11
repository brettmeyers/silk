#! /usr/bin/perl -w
# MD5: 7196f1be88bdcc1b296e4df76b0e92ad
# TEST: ./rwcount --bin-size=1800 --load-scheme=time-proportional ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=1800 --load-scheme=time-proportional $file{data}";
my $md5 = "7196f1be88bdcc1b296e4df76b0e92ad";

check_md5_output($md5, $cmd);
