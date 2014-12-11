#! /usr/bin/perl -w
# MD5: 3467899dbea8397db0fdb7a073ed69fc
# TEST: ./rwsetcat --network-structure=v6: ../../tests/set2-v6.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsetcat --network-structure=v6: $file{v6set2}";
my $md5 = "3467899dbea8397db0fdb7a073ed69fc";

check_md5_output($md5, $cmd);
