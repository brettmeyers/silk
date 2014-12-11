#! /usr/bin/perl -w
# MD5: e8cfbb3e6fd9f87ae1db589c4684a8b6
# TEST: ./rwpmapcat --no-cidr --country-codes

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwpmapcat --no-cidr --country-codes";
my $md5 = "e8cfbb3e6fd9f87ae1db589c4684a8b6";

check_md5_output($md5, $cmd);
