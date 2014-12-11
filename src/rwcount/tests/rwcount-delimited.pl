#! /usr/bin/perl -w
# MD5: 8950b28226714bd815cce45ab7ff935f
# TEST: ./rwcount --bin-size=3600 --load-scheme=1 --delimited=, ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=3600 --load-scheme=1 --delimited=, $file{data}";
my $md5 = "8950b28226714bd815cce45ab7ff935f";

check_md5_output($md5, $cmd);
