#! /usr/bin/perl -w
# MD5: 073a42d7b5a4d2b6a45a429ca6f7f200
# TEST: ./rwpmaplookup --country-codes --no-title --no-files 2001:db8:a:a::a:a

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my %file;
$file{v6_fake_cc} = get_data_or_exit77('v6_fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{v6_fake_cc}";
check_features(qw(ipv6));
my $cmd = "$rwpmaplookup --country-codes --no-title --no-files 2001:db8:a:a::a:a";
my $md5 = "073a42d7b5a4d2b6a45a429ca6f7f200";

check_md5_output($md5, $cmd);
