#! /usr/bin/perl -w
# MD5: 11151f02e3e150ffd4b2915cd8d4f190
# TEST: ./rwcount --bin-size=3600 --load-scheme=1 ../../tests/empty.rwf ../../tests/data.rwf ../../tests/empty.rwf ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwcount --bin-size=3600 --load-scheme=1 $file{empty} $file{data} $file{empty} $file{data}";
my $md5 = "11151f02e3e150ffd4b2915cd8d4f190";

check_md5_output($md5, $cmd);
