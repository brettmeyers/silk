#! /usr/bin/perl -w
# MD5: 2dfd16850cabde1372ab1270b9fc0386
# TEST: ./rwstats --fields=sip,dip --values=bytes --count=8 --top ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwstats --fields=sip,dip --values=bytes --count=8 --top $file{v6data}";
my $md5 = "2dfd16850cabde1372ab1270b9fc0386";

check_md5_output($md5, $cmd);
