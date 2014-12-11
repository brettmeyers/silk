#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwpdu2silk

use strict;
use SiLKTests;

my $rwpdu2silk = check_silk_app('rwpdu2silk');
my $cmd = "$rwpdu2silk";

exit (check_exit_status($cmd) ? 1 : 0);
