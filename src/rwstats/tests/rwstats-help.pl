#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwstats --help

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $cmd = "$rwstats --help";

exit (check_exit_status($cmd) ? 0 : 1);
