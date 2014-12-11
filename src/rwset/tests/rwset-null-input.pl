#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwset --sip-file=/dev/null </dev/null

use strict;
use SiLKTests;

my $rwset = check_silk_app('rwset');
my $cmd = "$rwset --sip-file=/dev/null </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
