#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwresolve </dev/null

use strict;
use SiLKTests;

my $rwresolve = check_silk_app('rwresolve');
my $cmd = "$rwresolve </dev/null";

exit (check_exit_status($cmd) ? 0 : 1);
