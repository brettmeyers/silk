#! /usr/bin/perl -w
# MD5: 092cbaa59bfa93d6917237c51452b83b
# TEST: ./rwuniq --fields=stime,etime --bin-time=3600 --values=bytes,packets,flows --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=stime,etime --bin-time=3600 --values=bytes,packets,flows --sort-output $file{data}";
my $md5 = "092cbaa59bfa93d6917237c51452b83b";

check_md5_output($md5, $cmd);
