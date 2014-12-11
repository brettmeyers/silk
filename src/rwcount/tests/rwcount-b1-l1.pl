#! /usr/bin/perl -w
# MD5: d56d49ba2cd419a0074be12b640777b3
# TEST: ./rwcount --bin-size=1 --load-scheme=1 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=1 --load-scheme=1 $file{data}";
my $md5 = "d56d49ba2cd419a0074be12b640777b3";

check_md5_output($md5, $cmd);
