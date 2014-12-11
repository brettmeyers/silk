#! /usr/bin/perl -w
# MD5: e42a0f34615ed531b0c2b939018fd81a
# TEST: ./rwset --sip-file=stdout ../../tests/data.rwf | ./rwsetmember 192.168.0.0/16 stdin

use strict;
use SiLKTests;

my $rwsetmember = check_silk_app('rwsetmember');
my $rwset = check_silk_app('rwset');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwsetmember 192.168.0.0/16 stdin";
my $md5 = "e42a0f34615ed531b0c2b939018fd81a";

check_md5_output($md5, $cmd);
