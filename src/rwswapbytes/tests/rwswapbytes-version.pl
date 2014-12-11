#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwswapbytes --version

use strict;
use SiLKTests;

my $rwswapbytes = check_silk_app('rwswapbytes');
my $cmd = "$rwswapbytes --version";

exit (check_exit_status($cmd) ? 0 : 1);
