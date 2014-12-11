#! /usr/bin/perl -w
# MD5: ed6dc3497209dc2632bc77a289fb794f
# TEST: ./rwstats --overall-stats ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --overall-stats $file{data}";
my $md5 = "ed6dc3497209dc2632bc77a289fb794f";

check_md5_output($md5, $cmd);
