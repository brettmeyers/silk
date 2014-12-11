#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwnetmask --help

use strict;
use SiLKTests;

my $rwnetmask = check_silk_app('rwnetmask');
my $cmd = "$rwnetmask --help";

exit (check_exit_status($cmd) ? 0 : 1);
