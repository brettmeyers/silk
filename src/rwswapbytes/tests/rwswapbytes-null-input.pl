#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwswapbytes --big-endian </dev/null

use strict;
use SiLKTests;

my $rwswapbytes = check_silk_app('rwswapbytes');
my $cmd = "$rwswapbytes --big-endian </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
