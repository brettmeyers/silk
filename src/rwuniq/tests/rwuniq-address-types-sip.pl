#! /usr/bin/perl -w
# MD5: 60287e588529eaaedbaf179440be10ae
# TEST: ./rwuniq --fields=stype --values=sip-distinct --delimited --ipv6=ignore --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{address_types} = get_data_or_exit77('address_types');
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwuniq --fields=stype --values=sip-distinct --delimited --ipv6=ignore --sort-output $file{data}";
my $md5 = "60287e588529eaaedbaf179440be10ae";

check_md5_output($md5, $cmd);
