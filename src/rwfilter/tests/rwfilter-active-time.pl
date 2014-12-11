#! /usr/bin/perl -w
# MD5: aaac158f9429dcee7dfd530c833d79f2
# TEST: ./rwfilter --active-time=2009/02/13:00:00-2009/02/13:00:05 --pass=stdout ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --active-time=2009/02/13:00:00-2009/02/13:00:05 --pass=stdout $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "aaac158f9429dcee7dfd530c833d79f2";

check_md5_output($md5, $cmd);
