#! /usr/bin/perl -w
# MD5: 3b718071df0052020d33785ce28245c0
# TEST: ../rwcut/rwcut --fields=sip --no-title --start-rec=1000 --num-rec=1000 --delimited ../../tests/data-v6.rwf | ./rwpmaplookup --country-codes=../../tests/fake-cc-v6.pmap --fields=value,input --delimited

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my $rwcut = check_silk_app('rwcut');
my %file;
$file{v6_fake_cc} = get_data_or_exit77('v6_fake_cc');
$file{v6data} = get_data_or_exit77('v6data');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{v6_fake_cc}";
check_features(qw(ipv6));
my $cmd = "$rwcut --fields=sip --no-title --start-rec=1000 --num-rec=1000 --delimited $file{v6data} | $rwpmaplookup --country-codes=$file{v6_fake_cc} --fields=value,input --delimited";
my $md5 = "3b718071df0052020d33785ce28245c0";

check_md5_output($md5, $cmd);
