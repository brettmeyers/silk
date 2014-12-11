#! /usr/bin/perl -w
# MD5: a6692ea30fff26f4b4f1a475faf6526a
# TEST: ./rwcount --bin-size=3600 --load-scheme=1 --column-separator=/ --no-final-delimiter ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=3600 --load-scheme=1 --column-separator=/ --no-final-delimiter $file{data}";
my $md5 = "a6692ea30fff26f4b4f1a475faf6526a";

check_md5_output($md5, $cmd);
