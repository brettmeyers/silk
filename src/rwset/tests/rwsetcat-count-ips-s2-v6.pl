#! /usr/bin/perl -w
# MD5: c9ac9ca70dbc4d3aaf20d168959d1fd4
# TEST: ./rwsetcat --count-ips ../../tests/set2-v6.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v6set2} = get_data_or_exit77('v6set2');
check_features(qw(ipset_v6));
my $cmd = "$rwsetcat --count-ips $file{v6set2}";
my $md5 = "c9ac9ca70dbc4d3aaf20d168959d1fd4";

check_md5_output($md5, $cmd);
