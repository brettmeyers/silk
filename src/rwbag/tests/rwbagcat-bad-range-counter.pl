#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwbagcat --mincounter=101 --maxcounter=99 /dev/null

use strict;
use SiLKTests;

my $rwbagcat = check_silk_app('rwbagcat');
my $cmd = "$rwbagcat --mincounter=101 --maxcounter=99 /dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
