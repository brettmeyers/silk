#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwnetmask --version

use strict;
use SiLKTests;

my $rwnetmask = check_silk_app('rwnetmask');
my $cmd = "$rwnetmask --version";

exit (check_exit_status($cmd) ? 0 : 1);
