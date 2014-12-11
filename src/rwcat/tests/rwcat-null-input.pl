#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwcat </dev/null

use strict;
use SiLKTests;

my $rwcat = check_silk_app('rwcat');
my $cmd = "$rwcat </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
