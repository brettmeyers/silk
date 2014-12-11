#! /usr/bin/perl -w
# MD5: e9bbf5bf9533b6b3e28722aaa1537b26
# TEST: ../rwcut/rwcut --fields=sip --ipv6-policy=ignore --no-title --start-rec=1000 --num-rec=1000 --delimited ../../tests/data.rwf | ../rwset/rwsetbuild | ./rwpmaplookup --country-codes=../../tests/fake-cc.pmap --fields=value,input --delimited --ipset-files

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my $rwcut = check_silk_app('rwcut');
my $rwsetbuild = check_silk_app('rwsetbuild');
my %file;
$file{address_types} = get_data_or_exit77('address_types');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$file{data} = get_data_or_exit77('data');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwcut --fields=sip --ipv6-policy=ignore --no-title --start-rec=1000 --num-rec=1000 --delimited $file{data} | $rwsetbuild | $rwpmaplookup --country-codes=$file{fake_cc} --fields=value,input --delimited --ipset-files";
my $md5 = "e9bbf5bf9533b6b3e28722aaa1537b26";

check_md5_output($md5, $cmd);
