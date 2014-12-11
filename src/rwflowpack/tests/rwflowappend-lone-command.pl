#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwflowappend

use strict;
use SiLKTests;

my $rwflowappend = check_silk_app('rwflowappend');
my $cmd = "$rwflowappend";

exit (check_exit_status($cmd) ? 1 : 0);
