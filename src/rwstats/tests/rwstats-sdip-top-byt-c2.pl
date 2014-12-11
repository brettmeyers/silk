#! /usr/bin/perl -w
# MD5: 16389d9154c30cf9e58ad7ea5afdeab4
# TEST: ./rwstats --fields=sip,dip --values=bytes --count=8 --top --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=sip,dip --values=bytes --count=8 --top --ipv6-policy=ignore $file{data}";
my $md5 = "16389d9154c30cf9e58ad7ea5afdeab4";

check_md5_output($md5, $cmd);
