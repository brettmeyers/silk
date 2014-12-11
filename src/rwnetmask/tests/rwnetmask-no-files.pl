#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwnetmask --sip=prefix-length=24

use strict;
use SiLKTests;

my $rwnetmask = check_silk_app('rwnetmask');
my $cmd = "$rwnetmask --sip=prefix-length=24";

exit (check_exit_status($cmd) ? 1 : 0);
