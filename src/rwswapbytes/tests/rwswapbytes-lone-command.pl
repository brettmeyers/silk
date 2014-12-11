#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwswapbytes

use strict;
use SiLKTests;

my $rwswapbytes = check_silk_app('rwswapbytes');
my $cmd = "$rwswapbytes";

exit (check_exit_status($cmd) ? 1 : 0);
