#! /usr/bin/perl -w
# MD5: 534e69235d92de3f308513d88d9a3e0f
# TEST: ./rwcombine ../../tests/empty.rwf --output-path=/dev/null --print-statistics=stdout

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwcombine $file{empty} --output-path=/dev/null --print-statistics=stdout";
my $md5 = "534e69235d92de3f308513d88d9a3e0f";

check_md5_output($md5, $cmd);
