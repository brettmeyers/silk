#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwset --sip-file=stdout ../../tests/empty.rwf | ./rwsetmember 10.x.x.x

use strict;
use SiLKTests;

my $rwsetmember = check_silk_app('rwsetmember');
my $rwset = check_silk_app('rwset');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwset --sip-file=stdout $file{empty} | $rwsetmember 10.x.x.x";

exit (check_exit_status($cmd) ? 1 : 0);
