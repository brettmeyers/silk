#! /usr/bin/perl -w
# MD5: 2e33ef5214a98839c39e61f2df32557b
# TEST: ../rwcut/rwcut --fields=sip --ipv6-policy=ignore --no-title --num-rec=1000 --delimited ../../tests/data.rwf | ../rwset/rwsetbuild | ./rwpmaplookup --ipset-files --address-types --no-final-delim

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my $rwcut = check_silk_app('rwcut');
my $rwsetbuild = check_silk_app('rwsetbuild');
my %file;
$file{address_types} = get_data_or_exit77('address_types');
$file{data} = get_data_or_exit77('data');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwcut --fields=sip --ipv6-policy=ignore --no-title --num-rec=1000 --delimited $file{data} | $rwsetbuild | $rwpmaplookup --ipset-files --address-types --no-final-delim";
my $md5 = "2e33ef5214a98839c39e61f2df32557b";

check_md5_output($md5, $cmd);
