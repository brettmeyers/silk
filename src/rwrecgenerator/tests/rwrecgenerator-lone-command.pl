#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwrecgenerator

use strict;
use SiLKTests;

my $rwrecgenerator = check_silk_app('rwrecgenerator');
my $cmd = "$rwrecgenerator";

exit (check_exit_status($cmd) ? 1 : 0);
