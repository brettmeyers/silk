#! /usr/bin/perl -w
# MD5: b96d11dae92cb09a9980a76ba94435a9
# TEST: ./rwcount --bin-size=3600 --load-scheme=end-spike --bin-slots ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=3600 --load-scheme=end-spike --bin-slots $file{data}";
my $md5 = "b96d11dae92cb09a9980a76ba94435a9";

check_md5_output($md5, $cmd);
