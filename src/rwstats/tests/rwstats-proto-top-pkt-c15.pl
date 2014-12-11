#! /usr/bin/perl -w
# MD5: 3b0ec45c576fde51676566b8f7e53fb0
# TEST: ./rwstats --fields=protocol --values=packets --count=15 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=protocol --values=packets --count=15 $file{data}";
my $md5 = "3b0ec45c576fde51676566b8f7e53fb0";

check_md5_output($md5, $cmd);
