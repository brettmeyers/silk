#! /usr/bin/perl -w
# MD5: 927efce0f9c245452d99f2a2fb9fcff5
# TEST: ../rwcut/rwcut --fields=sip --ipv6-policy=ignore --no-title --num-rec=1000 --delimited ../../tests/data.rwf | ./rwpmaplookup --address-types --no-final-delim

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my $rwcut = check_silk_app('rwcut');
my %file;
$file{address_types} = get_data_or_exit77('address_types');
$file{data} = get_data_or_exit77('data');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwcut --fields=sip --ipv6-policy=ignore --no-title --num-rec=1000 --delimited $file{data} | $rwpmaplookup --address-types --no-final-delim";
my $md5 = "927efce0f9c245452d99f2a2fb9fcff5";

check_md5_output($md5, $cmd);
