#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwfilter --all=/dev/null

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $cmd = "$rwfilter --all=/dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
