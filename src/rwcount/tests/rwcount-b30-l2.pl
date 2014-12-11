#! /usr/bin/perl -w
# MD5: 96bc0c8ef52aca21855f435742f8213d
# TEST: ./rwcount --bin-size=30 --load-scheme=2 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=30 --load-scheme=2 $file{data}";
my $md5 = "96bc0c8ef52aca21855f435742f8213d";

check_md5_output($md5, $cmd);
