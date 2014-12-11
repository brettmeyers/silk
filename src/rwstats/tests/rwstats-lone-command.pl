#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwstats

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $cmd = "$rwstats";

exit (check_exit_status($cmd) ? 1 : 0);
