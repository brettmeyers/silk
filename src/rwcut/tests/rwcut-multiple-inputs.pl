#! /usr/bin/perl -w
# MD5: 8a1930d83827f286e25b9d822ca55e46
# TEST: ./rwcut --fields=5 --delimited ../../tests/data.rwf ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=5 --delimited $file{data} $file{data}";
my $md5 = "8a1930d83827f286e25b9d822ca55e46";

check_md5_output($md5, $cmd);
