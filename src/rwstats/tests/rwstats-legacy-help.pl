#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwstats --legacy-help

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $cmd = "$rwstats --legacy-help";

exit (check_exit_status($cmd) ? 0 : 1);
