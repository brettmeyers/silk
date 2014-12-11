#! /usr/bin/perl -w
# MD5: ead5679c2b281268a7b8a717e5907429
# TEST: ls -1 ../../tests/empty.rwf ../../tests/data.rwf ../../tests/empty.rwf | ./rwcat --xargs | ../rwcut/rwcut --fields=1-15,20,21,26-29 --ipv6-policy=ignore --timestamp-format=epoch --ip-format=decimal --delimited

use strict;
use SiLKTests;

my $rwcat = check_silk_app('rwcat');
my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "ls -1 $file{empty} $file{data} $file{empty} | $rwcat --xargs | $rwcut --fields=1-15,20,21,26-29 --ipv6-policy=ignore --timestamp-format=epoch --ip-format=decimal --delimited";
my $md5 = "ead5679c2b281268a7b8a717e5907429";

check_md5_output($md5, $cmd);
