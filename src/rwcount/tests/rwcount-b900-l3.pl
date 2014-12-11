#! /usr/bin/perl -w
# MD5: 32ccef40e0a3b477b1747abb48d07499
# TEST: ./rwcount --bin-size=900 --load-scheme=3 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=900 --load-scheme=3 $file{data}";
my $md5 = "32ccef40e0a3b477b1747abb48d07499";

check_md5_output($md5, $cmd);
