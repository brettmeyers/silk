#! /usr/bin/perl -w
# MD5: b2fe7c5ff0637c79a504d73f6a4d3cea
# TEST: ./rwfilter --dtype=2 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{address_types} = get_data_or_exit77('address_types');
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwfilter --dtype=2 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "b2fe7c5ff0637c79a504d73f6a4d3cea";

check_md5_output($md5, $cmd);
