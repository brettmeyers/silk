#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwrecgenerator </dev/null

use strict;
use SiLKTests;

my $rwrecgenerator = check_silk_app('rwrecgenerator');
my $cmd = "$rwrecgenerator </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
