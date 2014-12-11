#! /usr/bin/perl -w
# MD5: 77e47dc7c5f5d8c16ecde4d5fb37bf86
# TEST: ./rwdedupe ../../tests/data-v6.rwf | ../rwuniq/rwuniq --fields=1-5 --ipv6-policy=force --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title

use strict;
use SiLKTests;

my $rwdedupe = check_silk_app('rwdedupe');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwdedupe $file{v6data} | $rwuniq --fields=1-5 --ipv6-policy=force --timestamp-format=epoch --values=bytes,packets,records,stime,etime --sort-output --delimited --no-title";
my $md5 = "77e47dc7c5f5d8c16ecde4d5fb37bf86";

check_md5_output($md5, $cmd);
