#! /usr/bin/perl -w
# MD5: 472226138dd507dd734ec73a3be7d9cf
# TEST: ./rwpmaplookup --country-codes --no-title --fields=start-block,end-block,value --no-files 10.10.10.10 10.200.200.200

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{address_types} = get_data_or_exit77('address_types');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwpmaplookup --country-codes --no-title --fields=start-block,end-block,value --no-files 10.10.10.10 10.200.200.200";
my $md5 = "472226138dd507dd734ec73a3be7d9cf";

check_md5_output($md5, $cmd);
