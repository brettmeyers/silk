#! /usr/bin/perl -w
# MD5: 9b205e3fae6dc993364dff8e30beb862
# TEST: ./rwcut --fields=stype,sip,dtype,dip,dtype --delimited --num-recs=10000 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
$file{address_types} = get_data_or_exit77('address_types');
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwcut --fields=stype,sip,dtype,dip,dtype --delimited --num-recs=10000 $file{data}";
my $md5 = "9b205e3fae6dc993364dff8e30beb862";

check_md5_output($md5, $cmd);
