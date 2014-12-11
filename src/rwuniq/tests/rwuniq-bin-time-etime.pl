#! /usr/bin/perl -w
# MD5: c413b77bbfee676c91c560d181f95aa2
# TEST: ./rwuniq --fields=etime --bin-time=3600 --values=bytes --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=etime --bin-time=3600 --values=bytes --sort-output $file{data}";
my $md5 = "c413b77bbfee676c91c560d181f95aa2";

check_md5_output($md5, $cmd);
