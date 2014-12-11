#! /usr/bin/perl -w
# MD5: f12b22035c6456a4d0572d9093ee0096
# TEST: ./rwstats --fields=dip --values=packets --threshold=25000 --top ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwstats --fields=dip --values=packets --threshold=25000 --top $file{v6data}";
my $md5 = "f12b22035c6456a4d0572d9093ee0096";

check_md5_output($md5, $cmd);
