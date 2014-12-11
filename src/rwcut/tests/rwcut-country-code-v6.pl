#! /usr/bin/perl -w
# MD5: 3e15b686173ef134e8f46a04a25ab067
# TEST: ./rwcut --fields=sip,scc,dip,dcc ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_fake_cc} = get_data_or_exit77('v6_fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{v6_fake_cc}";
my $cmd = "$rwcut --fields=sip,scc,dip,dcc $file{v6data}";
my $md5 = "3e15b686173ef134e8f46a04a25ab067";

check_md5_output($md5, $cmd);
