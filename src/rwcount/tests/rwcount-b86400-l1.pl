#! /usr/bin/perl -w
# MD5: 4427d543fa01929159d9419cc3a1110f
# TEST: ./rwcount --bin-size=86400 --load-scheme=start-spike --timestamp-format=epoch ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=86400 --load-scheme=start-spike --timestamp-format=epoch $file{data}";
my $md5 = "4427d543fa01929159d9419cc3a1110f";

check_md5_output($md5, $cmd);
