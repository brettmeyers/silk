#! /usr/bin/perl -w
# MD5: fa4e3f687b05a84776ee920a272b1985
# TEST: ./rwfilter --output-index=10 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --output-index=10 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "fa4e3f687b05a84776ee920a272b1985";

check_md5_output($md5, $cmd);
