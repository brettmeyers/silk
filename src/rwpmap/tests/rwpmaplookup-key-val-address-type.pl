#! /usr/bin/perl -w
# MD5: 044a3e66d89d56fe3e3779f8b7ed5505
# TEST: ./rwpmaplookup --address-types --column-sep=, --no-files 10.10.10.10

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{address_types} = get_data_or_exit77('address_types');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwpmaplookup --address-types --column-sep=, --no-files 10.10.10.10";
my $md5 = "044a3e66d89d56fe3e3779f8b7ed5505";

check_md5_output($md5, $cmd);
