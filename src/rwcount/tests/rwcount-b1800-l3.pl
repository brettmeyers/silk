#! /usr/bin/perl -w
# MD5: 12285910e0c46b1d0c10fb648eed3bfe
# TEST: ./rwcount --bin-size=1800 --load-scheme=middle-spike ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=1800 --load-scheme=middle-spike $file{data}";
my $md5 = "12285910e0c46b1d0c10fb648eed3bfe";

check_md5_output($md5, $cmd);
