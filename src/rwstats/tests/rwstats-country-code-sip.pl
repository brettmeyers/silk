#! /usr/bin/perl -w
# MD5: 437ef8f2ff35ebe3a061c4ce785a0cc8
# TEST: ./rwstats --fields=scc --values=sip-distinct --ipv6=ignore --count=10 --no-percent ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
my $cmd = "$rwstats --fields=scc --values=sip-distinct --ipv6=ignore --count=10 --no-percent $file{data}";
my $md5 = "437ef8f2ff35ebe3a061c4ce785a0cc8";

check_md5_output($md5, $cmd);
