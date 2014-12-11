#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwflowappend --version

use strict;
use SiLKTests;

my $rwflowappend = check_silk_app('rwflowappend');
my $cmd = "$rwflowappend --version";

exit (check_exit_status($cmd) ? 0 : 1);
