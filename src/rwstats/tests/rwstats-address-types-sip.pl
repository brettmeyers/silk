#! /usr/bin/perl -w
# MD5: 237e3d2c8a81954088f378d102e358ca
# TEST: ./rwstats --fields=stype --values=sip-distinct --delimited --ipv6=ignore --count=2 --no-percent ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
$file{address_types} = get_data_or_exit77('address_types');
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwstats --fields=stype --values=sip-distinct --delimited --ipv6=ignore --count=2 --no-percent $file{data}";
my $md5 = "237e3d2c8a81954088f378d102e358ca";

check_md5_output($md5, $cmd);
