#! /usr/bin/perl -w
# MD5: c6d725070e61173fffc6bf9a36f897cc
# TEST: ./rwstats --fields=sip --percentage=4 --top --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=sip --percentage=4 --top --ipv6-policy=ignore $file{data}";
my $md5 = "c6d725070e61173fffc6bf9a36f897cc";

check_md5_output($md5, $cmd);
