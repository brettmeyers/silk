#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwappend --version

use strict;
use SiLKTests;

my $rwappend = check_silk_app('rwappend');
my $cmd = "$rwappend --version";

exit (check_exit_status($cmd) ? 0 : 1);
