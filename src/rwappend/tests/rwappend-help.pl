#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwappend --help

use strict;
use SiLKTests;

my $rwappend = check_silk_app('rwappend');
my $cmd = "$rwappend --help";

exit (check_exit_status($cmd) ? 0 : 1);
