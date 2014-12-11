#! /usr/bin/perl -w
# MD5: fe7240e705f3b51b05b3b56bca55379b
# TEST: ./rwpmaplookup --country-codes --no-title --no-files 10.10.10.10

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{address_types} = get_data_or_exit77('address_types');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwpmaplookup --country-codes --no-title --no-files 10.10.10.10";
my $md5 = "fe7240e705f3b51b05b3b56bca55379b";

check_md5_output($md5, $cmd);
