#! /usr/bin/perl -w
# MD5: 0c9ac3d105993e5801a81e1d18ba449e
# TEST: ./rwsort --fields=5,1,3,2,4 ../../tests/data.rwf | ../rwuniq/rwuniq --fields=1-5 --ipv6-policy=ignore --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=5,1,3,2,4 $file{data} | $rwuniq --fields=1-5 --ipv6-policy=ignore --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title";
my $md5 = "0c9ac3d105993e5801a81e1d18ba449e";

check_md5_output($md5, $cmd);
