#! /usr/bin/perl -w
# MD5: e21fbe6bc4551a58713a8a538420ecfe
# TEST: ./rwswapbytes --little-endian ../../tests/data-v6.rwf - | ../rwcut/rwcut --fields=1-15,26-29 --timestamp-format=epoch

use strict;
use SiLKTests;

my $rwswapbytes = check_silk_app('rwswapbytes');
my $rwcut = check_silk_app('rwcut');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipv6));
my $cmd = "$rwswapbytes --little-endian $file{v6data} - | $rwcut --fields=1-15,26-29 --timestamp-format=epoch";
my $md5 = "e21fbe6bc4551a58713a8a538420ecfe";

check_md5_output($md5, $cmd);
