#! /usr/bin/perl -w
# MD5: a724f33765cf347331d8bdd553a1c256
# TEST: ./rwcombine --buffer-size=2m --max-idle-time=0.002 ../../tests/data-v6.rwf | ../rwuniq/rwuniq --fields=1-5 --ipv6-policy=force --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
my $cmd = "$rwcombine --buffer-size=2m --max-idle-time=0.002 $file{v6data} | $rwuniq --fields=1-5 --ipv6-policy=force --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title";
my $md5 = "a724f33765cf347331d8bdd553a1c256";

check_md5_output($md5, $cmd);
