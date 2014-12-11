#! /usr/bin/perl -w
# MD5: 4d68466b8f35b4cc95929a4bfac81da7
# TEST: ./rwdedupe --ignore-fields=stime ../../tests/data-v6.rwf ../../tests/empty.rwf | ../rwuniq/rwuniq --fields=1-5 --ipv6-policy=force --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title

use strict;
use SiLKTests;

my $rwdedupe = check_silk_app('rwdedupe');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwdedupe --ignore-fields=stime $file{v6data} $file{empty} | $rwuniq --fields=1-5 --ipv6-policy=force --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title";
my $md5 = "4d68466b8f35b4cc95929a4bfac81da7";

check_md5_output($md5, $cmd);
