#! /usr/bin/perl -w
# MD5: 6dec60f5a4f696640dbda8a1ec7ee5b0
# TEST: ./rwcut --fields=sip,scc,dip,dcc --ipv6=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
my $cmd = "$rwcut --fields=sip,scc,dip,dcc --ipv6=ignore $file{data}";
my $md5 = "6dec60f5a4f696640dbda8a1ec7ee5b0";

check_md5_output($md5, $cmd);
