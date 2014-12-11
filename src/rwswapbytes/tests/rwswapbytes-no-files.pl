#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwswapbytes --big-endian

use strict;
use SiLKTests;

my $rwswapbytes = check_silk_app('rwswapbytes');
my $cmd = "$rwswapbytes --big-endian";

exit (check_exit_status($cmd) ? 1 : 0);
