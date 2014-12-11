#! /usr/bin/perl -w
# MD5: b49b8fd109db835d2332d87274e4aeb5
# TEST: ./rwdedupe --ignore-fields=stime ../../tests/data.rwf ../../tests/empty.rwf | ../rwuniq/rwuniq --fields=1-5 --ipv6-policy=ignore --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title

use strict;
use SiLKTests;

my $rwdedupe = check_silk_app('rwdedupe');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwdedupe --ignore-fields=stime $file{data} $file{empty} | $rwuniq --fields=1-5 --ipv6-policy=ignore --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title";
my $md5 = "b49b8fd109db835d2332d87274e4aeb5";

check_md5_output($md5, $cmd);
