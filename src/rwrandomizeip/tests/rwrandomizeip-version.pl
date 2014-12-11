#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwrandomizeip --version

use strict;
use SiLKTests;

my $rwrandomizeip = check_silk_app('rwrandomizeip');
my $cmd = "$rwrandomizeip --version";

exit (check_exit_status($cmd) ? 0 : 1);
