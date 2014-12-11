#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsetmember 10.x.x.x </dev/null

use strict;
use SiLKTests;

my $rwsetmember = check_silk_app('rwsetmember');
my $cmd = "$rwsetmember 10.x.x.x </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
