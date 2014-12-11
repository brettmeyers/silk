#! /usr/bin/perl -w
# MD5: 60287e588529eaaedbaf179440be10ae
# TEST: ./rwsort --fields=stype ../../tests/data.rwf | ../rwuniq/rwuniq --fields=stype --values=sip-distinct --delimited --ipv6=ignore --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{address_types} = get_data_or_exit77('address_types');
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwsort --fields=stype $file{data} | $rwuniq --fields=stype --values=sip-distinct --delimited --ipv6=ignore --presorted-input";
my $md5 = "60287e588529eaaedbaf179440be10ae";

check_md5_output($md5, $cmd);
