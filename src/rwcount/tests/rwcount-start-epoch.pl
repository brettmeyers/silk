#! /usr/bin/perl -w
# MD5: aac37bddeb60bb860c996649ccba5d26
# TEST: ./rwcount --bin-size=3600 --load-scheme=bin-uniform --start-time=2009/02/12T20:30:00 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=3600 --load-scheme=bin-uniform --start-time=2009/02/12T20:30:00 $file{data}";
my $md5 = "aac37bddeb60bb860c996649ccba5d26";

check_md5_output($md5, $cmd);
