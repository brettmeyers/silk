#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwbag --sport-flows=/dev/null </dev/null

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $cmd = "$rwbag --sport-flows=/dev/null </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
