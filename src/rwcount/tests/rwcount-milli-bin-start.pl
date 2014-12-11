#! /usr/bin/perl -w
# MD5: 6767ae7692f04673bae59b15df1b1556
# TEST: ./rwcount --bin-size=0.500 --skip-zero --load-scheme=1 --start-time=2009/02/14T20:00:00 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=0.500 --skip-zero --load-scheme=1 --start-time=2009/02/14T20:00:00 $file{data}";
my $md5 = "6767ae7692f04673bae59b15df1b1556";

check_md5_output($md5, $cmd);
