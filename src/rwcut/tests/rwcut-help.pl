#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwcut --help

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my $cmd = "$rwcut --help";

exit (check_exit_status($cmd) ? 0 : 1);
