#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwcount </dev/null

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my $cmd = "$rwcount </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
