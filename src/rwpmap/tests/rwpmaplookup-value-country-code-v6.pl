#! /usr/bin/perl -w
# MD5: df35e35989e712fa96925b955d6409ac
# TEST: ./rwpmaplookup --country-codes=../../tests/fake-cc-v6.pmap --fields=value --no-title -delim --no-files 2001:db8:a:a::a:a

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{v6_fake_cc} = get_data_or_exit77('v6_fake_cc');
check_features(qw(ipv6));
my $cmd = "$rwpmaplookup --country-codes=$file{v6_fake_cc} --fields=value --no-title -delim --no-files 2001:db8:a:a::a:a";
my $md5 = "df35e35989e712fa96925b955d6409ac";

check_md5_output($md5, $cmd);
