#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwnetmask

use strict;
use SiLKTests;

my $rwnetmask = check_silk_app('rwnetmask');
my $cmd = "$rwnetmask";

exit (check_exit_status($cmd) ? 1 : 0);
