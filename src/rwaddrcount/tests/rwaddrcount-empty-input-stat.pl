#! /usr/bin/perl -w
# MD5: a07d203e19e6f7d54fb06decbeca2171
# TEST: ./rwaddrcount --print-stat ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwaddrcount --print-stat $file{empty}";
my $md5 = "a07d203e19e6f7d54fb06decbeca2171";

check_md5_output($md5, $cmd);
