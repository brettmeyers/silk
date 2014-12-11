#! /usr/bin/perl -w
# MD5: a2d179328fa4fd2a625a1582b7dd5483
# TEST: ./rwcut --fields=5 --delimited ../../tests/data-v6.rwf ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwcut --fields=5 --delimited $file{v6data} $file{data}";
my $md5 = "a2d179328fa4fd2a625a1582b7dd5483";

check_md5_output($md5, $cmd);
