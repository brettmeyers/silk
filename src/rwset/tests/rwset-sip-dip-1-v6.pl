#! /usr/bin/perl -w
# MD5: ed9c24e84ad0cb789748607d251a8145
# TEST: ./rwset --sip=stdout --dip=/dev/null ../../tests/data-v6.rwf | ./rwsetcat --cidr-blocks=0 --ip-format=hexadecimal

use strict;
use SiLKTests;

my $rwset = check_silk_app('rwset');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipset_v6));
my $cmd = "$rwset --sip=stdout --dip=/dev/null $file{v6data} | $rwsetcat --cidr-blocks=0 --ip-format=hexadecimal";
my $md5 = "ed9c24e84ad0cb789748607d251a8145";

check_md5_output($md5, $cmd);
