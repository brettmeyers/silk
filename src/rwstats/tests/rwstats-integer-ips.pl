#! /usr/bin/perl -w
# MD5: 9c7a742c3348d750d56e0e39afb553ad
# TEST: ./rwstats --fields=sip,dip --values=bytes --count=8 --top --ip-format=decimal --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=sip,dip --values=bytes --count=8 --top --ip-format=decimal --ipv6-policy=ignore $file{data}";
my $md5 = "9c7a742c3348d750d56e0e39afb553ad";

check_md5_output($md5, $cmd);
