#! /usr/bin/perl -w
# MD5: cba4ecd0fe9760b61f5ccdce3cde18a0
# TEST: ./rwstats --fields=proto --values=distinct:sip,distinct:dip --count=5 --no-percent ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwstats --fields=proto --values=distinct:sip,distinct:dip --count=5 --no-percent $file{v6data}";
my $md5 = "cba4ecd0fe9760b61f5ccdce3cde18a0";

check_md5_output($md5, $cmd);
