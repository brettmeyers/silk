#! /usr/bin/perl -w
# MD5: 437ef8f2ff35ebe3a061c4ce785a0cc8
# TEST: ./rwstats --fields=scc --values=sip-distinct --count=10 --no-percent ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
$file{v6_fake_cc} = get_data_or_exit77('v6_fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{v6_fake_cc}";
my $cmd = "$rwstats --fields=scc --values=sip-distinct --count=10 --no-percent $file{v6data}";
my $md5 = "437ef8f2ff35ebe3a061c4ce785a0cc8";

check_md5_output($md5, $cmd);
