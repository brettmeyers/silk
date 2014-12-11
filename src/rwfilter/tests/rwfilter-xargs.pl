#! /usr/bin/perl -w
# MD5: dd4ad291c05df4f4cc2ca9dfe918c876
# TEST: ls -1 ../../tests/data.rwf ../../tests/data.rwf ../../tests/data.rwf | ./rwfilter --xargs=stdin --proto=17 --pass=stdout | ../rwuniq/rwuniq --fields=1-5 --ipv6-policy=ignore --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-titles

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "ls -1 $file{data} $file{data} $file{data} | $rwfilter --xargs=stdin --proto=17 --pass=stdout | $rwuniq --fields=1-5 --ipv6-policy=ignore --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-titles";
my $md5 = "dd4ad291c05df4f4cc2ca9dfe918c876";

check_md5_output($md5, $cmd);
