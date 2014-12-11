#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwstats --fields=sip --count=10

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $cmd = "$rwstats --fields=sip --count=10";

exit (check_exit_status($cmd) ? 1 : 0);
