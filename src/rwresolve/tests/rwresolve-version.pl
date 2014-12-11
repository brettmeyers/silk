#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwresolve --version

use strict;
use SiLKTests;

my $rwresolve = check_silk_app('rwresolve');
my $cmd = "$rwresolve --version";

exit (check_exit_status($cmd) ? 0 : 1);
