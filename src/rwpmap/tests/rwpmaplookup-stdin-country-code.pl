#! /usr/bin/perl -w
# MD5: 75956e032078476702336e2ca3228577
# TEST: ../rwcut/rwcut --fields=sip --ipv6-policy=ignore --no-title --start-rec=1000 --num-rec=1000 --delimited ../../tests/data.rwf | ./rwpmaplookup --country-codes=../../tests/fake-cc.pmap --fields=value,input --delimited

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my $rwcut = check_silk_app('rwcut');
my %file;
$file{address_types} = get_data_or_exit77('address_types');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$file{data} = get_data_or_exit77('data');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwcut --fields=sip --ipv6-policy=ignore --no-title --start-rec=1000 --num-rec=1000 --delimited $file{data} | $rwpmaplookup --country-codes=$file{fake_cc} --fields=value,input --delimited";
my $md5 = "75956e032078476702336e2ca3228577";

check_md5_output($md5, $cmd);
