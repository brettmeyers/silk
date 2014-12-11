#! /usr/bin/perl -w
# MD5: 69528e35384b0f8b59c20635fd734a9a
# TEST: ./rwcut --fields=in,out,nhip --delimited=, --tail-recs=2000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=in,out,nhip --delimited=, --tail-recs=2000 $file{data}";
my $md5 = "69528e35384b0f8b59c20635fd734a9a";

check_md5_output($md5, $cmd);
