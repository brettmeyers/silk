#! /usr/bin/perl -w
# MD5: 85668818573ea8b74332cf755b131ac1
# TEST: ./rwtotal --sport --min-byte=2000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --sport --min-byte=2000 $file{data}";
my $md5 = "85668818573ea8b74332cf755b131ac1";

check_md5_output($md5, $cmd);
