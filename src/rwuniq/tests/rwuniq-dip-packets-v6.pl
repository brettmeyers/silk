#! /usr/bin/perl -w
# MD5: fc6357a25897be21e1c62658ef20f88d
# TEST: ./rwuniq --fields=2 --values=packets --ipv6-policy=force --sort-output ../../tests/data-v6.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwuniq --fields=2 --values=packets --ipv6-policy=force --sort-output $file{v6data}";
my $md5 = "fc6357a25897be21e1c62658ef20f88d";

check_md5_output($md5, $cmd);
