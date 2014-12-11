#! /usr/bin/perl -w
# MD5: 39817571ebd011f54fa48c1c58fb99a6
# TEST: ./rwcount --bin-size=1800 --load-scheme=maximum-volume ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=1800 --load-scheme=maximum-volume $file{data}";
my $md5 = "39817571ebd011f54fa48c1c58fb99a6";

check_md5_output($md5, $cmd);
