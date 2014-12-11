#! /usr/bin/perl -w
# MD5: ca0738b33875ed283d0a4363cb2c7a5a
# TEST: ./rwcombine ../../tests/data.rwf | ../rwuniq/rwuniq --fields=1-5 --ipv6-policy=ignore --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcombine $file{data} | $rwuniq --fields=1-5 --ipv6-policy=ignore --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title";
my $md5 = "ca0738b33875ed283d0a4363cb2c7a5a";

check_md5_output($md5, $cmd);
