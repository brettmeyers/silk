#! /usr/bin/perl -w
# MD5: 0badd879611422b214d8c53e19b4f2a3
# TEST: ./rwpmaplookup --address-types=../../tests/address_types.pmap --fields=value --no-title -delim --no-files 10.10.10.10

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{address_types} = get_data_or_exit77('address_types');
my $cmd = "$rwpmaplookup --address-types=$file{address_types} --fields=value --no-title -delim --no-files 10.10.10.10";
my $md5 = "0badd879611422b214d8c53e19b4f2a3";

check_md5_output($md5, $cmd);
