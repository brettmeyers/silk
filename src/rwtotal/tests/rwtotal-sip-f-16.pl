#! /usr/bin/perl -w
# MD5: ca5b8ab3f2e6d39949dff13d9caf799a
# TEST: ./rwtotal --sip-first-16 --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sip-first-16 --skip-zero $file{data}";
my $md5 = "ca5b8ab3f2e6d39949dff13d9caf799a";

check_md5_output($md5, $cmd);
