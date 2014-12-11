#! /usr/bin/perl -w
# MD5: e38dfd456f503c8306e185305973c442
# TEST: cat ../../tests/data.rwf | ./rwswapbytes --big - - | ../rwcut/rwcut --fields=1-15,26-29 --ip-format=decimal --timestamp-format=epoch --ipv6-policy=ignore

use strict;
use SiLKTests;

my $rwswapbytes = check_silk_app('rwswapbytes');
my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "cat $file{data} | $rwswapbytes --big - - | $rwcut --fields=1-15,26-29 --ip-format=decimal --timestamp-format=epoch --ipv6-policy=ignore";
my $md5 = "e38dfd456f503c8306e185305973c442";

check_md5_output($md5, $cmd);
