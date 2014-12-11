#! /usr/bin/perl -w
# MD5: a34092df42f77ee0961b10a19d5782c3
# TEST: ./rwcut --all-fields --delimited ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwcut --all-fields --delimited $file{v6data}";
my $md5 = "a34092df42f77ee0961b10a19d5782c3";

check_md5_output($md5, $cmd);
