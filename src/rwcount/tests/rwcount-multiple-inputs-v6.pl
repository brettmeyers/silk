#! /usr/bin/perl -w
# MD5: 11151f02e3e150ffd4b2915cd8d4f190
# TEST: ./rwcount --bin-size=3600 --load-scheme=1 ../../tests/data-v6.rwf ../../tests/empty.rwf ../../tests/data-v6.rwf ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{empty} = get_data_or_exit77('empty');
check_features(qw(ipv6));
my $cmd = "$rwcount --bin-size=3600 --load-scheme=1 $file{v6data} $file{empty} $file{v6data} $file{empty}";
my $md5 = "11151f02e3e150ffd4b2915cd8d4f190";

check_md5_output($md5, $cmd);
