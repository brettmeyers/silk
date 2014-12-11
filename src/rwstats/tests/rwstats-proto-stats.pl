#! /usr/bin/perl -w
# MD5: 886ac7910929a3e83305568a00c88cff
# TEST: ./rwstats --detail-proto-stats=1 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --detail-proto-stats=1 $file{data}";
my $md5 = "886ac7910929a3e83305568a00c88cff";

check_md5_output($md5, $cmd);
