#! /usr/bin/perl -w
# MD5: ca650f5ea22b234ac67a890a9d2e4489
# TEST: ./rwstats --fields=dcc --values=dip-distinct --ipv6=ignore --count=10 --no-percent ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
my $cmd = "$rwstats --fields=dcc --values=dip-distinct --ipv6=ignore --count=10 --no-percent $file{data}";
my $md5 = "ca650f5ea22b234ac67a890a9d2e4489";

check_md5_output($md5, $cmd);
