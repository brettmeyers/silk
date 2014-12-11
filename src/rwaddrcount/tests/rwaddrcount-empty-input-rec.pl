#! /usr/bin/perl -w
# MD5: 371b7969780b88503b72213edaac3818
# TEST: ./rwaddrcount --print-rec ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwaddrcount --print-rec $file{empty}";
my $md5 = "371b7969780b88503b72213edaac3818";

check_md5_output($md5, $cmd);
