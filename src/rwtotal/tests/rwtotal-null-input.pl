#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwtotal --sport </dev/null

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my $cmd = "$rwtotal --sport </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
