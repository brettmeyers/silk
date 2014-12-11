#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwaddrcount --print-recs </dev/null

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my $cmd = "$rwaddrcount --print-recs </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
