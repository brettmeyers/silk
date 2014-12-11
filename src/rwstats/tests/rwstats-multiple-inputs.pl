#! /usr/bin/perl -w
# MD5: 6acc4b5d8569198f2f414125d6239dfd
# TEST: ./rwstats --fields=dport --values=bytes --count=20 --top ../../tests/data.rwf ../../tests/empty.rwf ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwstats --fields=dport --values=bytes --count=20 --top $file{data} $file{empty} $file{data}";
my $md5 = "6acc4b5d8569198f2f414125d6239dfd";

check_md5_output($md5, $cmd);
