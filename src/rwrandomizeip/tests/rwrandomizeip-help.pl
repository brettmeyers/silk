#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwrandomizeip --help

use strict;
use SiLKTests;

my $rwrandomizeip = check_silk_app('rwrandomizeip');
my $cmd = "$rwrandomizeip --help";

exit (check_exit_status($cmd) ? 0 : 1);
