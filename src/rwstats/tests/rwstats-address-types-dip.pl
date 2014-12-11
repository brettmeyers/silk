#! /usr/bin/perl -w
# MD5: 9a6347309bd9a72df9411eaa8e74d364
# TEST: ./rwstats --fields=dtype --values=dip-distinct --delimited --ipv6=ignore --count=2 --no-percent ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
$file{address_types} = get_data_or_exit77('address_types');
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwstats --fields=dtype --values=dip-distinct --delimited --ipv6=ignore --count=2 --no-percent $file{data}";
my $md5 = "9a6347309bd9a72df9411eaa8e74d364";

check_md5_output($md5, $cmd);
