#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwcombine

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my $cmd = "$rwcombine";

exit (check_exit_status($cmd) ? 1 : 0);
