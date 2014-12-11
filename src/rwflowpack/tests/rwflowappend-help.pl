#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwflowappend --help

use strict;
use SiLKTests;

my $rwflowappend = check_silk_app('rwflowappend');
my $cmd = "$rwflowappend --help";

exit (check_exit_status($cmd) ? 0 : 1);
