#! /usr/bin/perl -w
# MD5: varies
# TEST: ./rwfileinfo --fields=1,5-6 --no-title ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwfileinfo = check_silk_app('rwfileinfo');
my %file;
$file{data} = get_data_or_exit77('data');

my $cmd = "$rwfileinfo --fields=1,5-6 --no-title $file{data}";
my $md5 = (($SiLKTests::SK_ENABLE_IPV6)
           ? "0cd4e1fc5a2bca6e84598944cabf3b8a"
           : "e32b56f4634d68aa52522e61917e5b1a");

check_md5_output($md5, $cmd);
