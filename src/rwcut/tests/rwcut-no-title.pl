#! /usr/bin/perl -w
# MD5: 6c3cabf4f227aa44239a0c45ce137573
# TEST: ./rwcut --fields=5,4,3 --no-title < ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=5,4,3 --no-title < $file{data}";
my $md5 = "6c3cabf4f227aa44239a0c45ce137573";

check_md5_output($md5, $cmd);
