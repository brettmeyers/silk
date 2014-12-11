#! /usr/bin/perl -w
# MD5: 8fb9d8cf9d4ce725f623156e25c50a7d
# TEST: ./rwstats --fields=sport,sip --values=packets,bytes --count=10 --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=sport,sip --values=packets,bytes --count=10 --ipv6-policy=ignore $file{data}";
my $md5 = "8fb9d8cf9d4ce725f623156e25c50a7d";

check_md5_output($md5, $cmd);
