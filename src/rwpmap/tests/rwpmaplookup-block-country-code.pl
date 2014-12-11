#! /usr/bin/perl -w
# MD5: 73718355b5cf1bdc5c584f9d89975059
# TEST: ./rwpmaplookup --country-codes=../../tests/fake-cc.pmap --no-title --fields=block,key,value --no-files 10.10.10.10 10.200.200.200

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{address_types} = get_data_or_exit77('address_types');
$file{fake_cc} = get_data_or_exit77('fake_cc');
my $cmd = "$rwpmaplookup --country-codes=$file{fake_cc} --no-title --fields=block,key,value --no-files 10.10.10.10 10.200.200.200";
my $md5 = "73718355b5cf1bdc5c584f9d89975059";

check_md5_output($md5, $cmd);
