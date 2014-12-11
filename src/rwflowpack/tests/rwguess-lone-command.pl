#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwguess

use strict;
use SiLKTests;

my $rwguess = check_silk_app('rwguess');
my $cmd = "$rwguess";

exit (check_exit_status($cmd) ? 1 : 0);
