#! /usr/bin/perl -w
# MD5: 53fcf40ef3cb674c514a38b93711d0d7
# TEST: ./rwfilter --icmp-code=3 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --icmp-code=3 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "53fcf40ef3cb674c514a38b93711d0d7";

check_md5_output($md5, $cmd);
