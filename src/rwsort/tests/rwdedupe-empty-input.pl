#! /usr/bin/perl -w
# MD5: d41d8cd98f00b204e9800998ecf8427e
# TEST: ./rwdedupe ../../tests/empty.rwf | ../rwuniq/rwuniq --fields=1-5 --ipv6-policy=ignore --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title

use strict;
use SiLKTests;

my $rwdedupe = check_silk_app('rwdedupe');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwdedupe $file{empty} | $rwuniq --fields=1-5 --ipv6-policy=ignore --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title";
my $md5 = "d41d8cd98f00b204e9800998ecf8427e";

check_md5_output($md5, $cmd);
