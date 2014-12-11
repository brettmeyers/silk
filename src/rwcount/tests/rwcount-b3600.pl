#! /usr/bin/perl -w
# MD5: 518f7eaca8f9d9e38a86681a1df7e414
# TEST: ./rwcount --bin-size=3600 --no-title ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=3600 --no-title $file{data}";
my $md5 = "518f7eaca8f9d9e38a86681a1df7e414";

check_md5_output($md5, $cmd);
