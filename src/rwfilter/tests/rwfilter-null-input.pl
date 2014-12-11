#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwfilter --input-pipe=/dev/null --all=/dev/null

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $cmd = "$rwfilter --input-pipe=/dev/null --all=/dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
