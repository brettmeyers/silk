#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwguess --help

use strict;
use SiLKTests;

my $rwguess = check_silk_app('rwguess');
my $cmd = "$rwguess --help";

exit (check_exit_status($cmd) ? 0 : 1);
