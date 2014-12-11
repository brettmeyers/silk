#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwrandomizeip

use strict;
use SiLKTests;

my $rwrandomizeip = check_silk_app('rwrandomizeip');
my $cmd = "$rwrandomizeip";

exit (check_exit_status($cmd) ? 1 : 0);
