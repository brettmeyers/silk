#! /usr/bin/perl -w
# MD5: 4e1b080408698c01f9f7b3bf0568cbda
# TEST: ./rwset --sip-file=stdout ../../tests/data-v6.rwf | ./rwsetmember --count 2001:db8:c0:a8::x:x -

use strict;
use SiLKTests;

my $rwsetmember = check_silk_app('rwsetmember');
my $rwset = check_silk_app('rwset');
my %file;
$file{v6data} = get_data_or_exit77('v6data');
check_features(qw(ipset_v6));
my $cmd = "$rwset --sip-file=stdout $file{v6data} | $rwsetmember --count 2001:db8:c0:a8::x:x -";
my $md5 = "4e1b080408698c01f9f7b3bf0568cbda";

check_md5_output($md5, $cmd);
