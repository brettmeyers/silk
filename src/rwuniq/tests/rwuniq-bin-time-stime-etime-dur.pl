#! /usr/bin/perl -w
# MD5: 9a1ef7d81732fb3a52fc2e335c576941
# TEST: ./rwuniq --fields=stime,etime,dur --bin-time=3600 --values=bytes,packets,flows --sort-output ../../tests/data.rwf 2>/dev/null

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=stime,etime,dur --bin-time=3600 --values=bytes,packets,flows --sort-output $file{data} 2>/dev/null";
my $md5 = "9a1ef7d81732fb3a52fc2e335c576941";

check_md5_output($md5, $cmd);
