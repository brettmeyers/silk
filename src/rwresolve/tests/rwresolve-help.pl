#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwresolve --help

use strict;
use SiLKTests;

my $rwresolve = check_silk_app('rwresolve');
my $cmd = "$rwresolve --help";

exit (check_exit_status($cmd) ? 0 : 1);
