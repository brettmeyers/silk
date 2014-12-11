#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwappend

use strict;
use SiLKTests;

my $rwappend = check_silk_app('rwappend');
my $cmd = "$rwappend";

exit (check_exit_status($cmd) ? 1 : 0);
