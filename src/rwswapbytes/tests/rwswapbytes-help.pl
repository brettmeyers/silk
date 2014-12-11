#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwswapbytes --help

use strict;
use SiLKTests;

my $rwswapbytes = check_silk_app('rwswapbytes');
my $cmd = "$rwswapbytes --help";

exit (check_exit_status($cmd) ? 0 : 1);
