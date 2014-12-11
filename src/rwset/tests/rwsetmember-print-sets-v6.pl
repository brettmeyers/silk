#! /usr/bin/perl -w
# MD5: e42a0f34615ed531b0c2b939018fd81a
# TEST: ./rwset --sip-file=stdout ../../tests/data-v6.rwf | ./rwsetmember 2001:db8:c0:a8::/64 stdin

use strict;
use SiLKTests;

my $rwsetmember = check_silk_app('rwsetmember');
my $rwset = check_silk_app('rwset');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipset_v6));
my $cmd = "$rwset --sip-file=stdout $file{v6data} | $rwsetmember 2001:db8:c0:a8::/64 stdin";
my $md5 = "e42a0f34615ed531b0c2b939018fd81a";

check_md5_output($md5, $cmd);
