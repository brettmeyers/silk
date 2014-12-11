#! /usr/bin/perl -w
# MD5: dcf63bbf88f5acc1ad181ae316d79832
# TEST: ./rwcount --bin-size=1800 --load-scheme=minimum-volume ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=1800 --load-scheme=minimum-volume $file{data}";
my $md5 = "dcf63bbf88f5acc1ad181ae316d79832";

check_md5_output($md5, $cmd);
