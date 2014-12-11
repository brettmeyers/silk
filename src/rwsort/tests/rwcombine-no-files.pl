#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwcombine --ignore-fields=1

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my $cmd = "$rwcombine --ignore-fields=1";

exit (check_exit_status($cmd) ? 1 : 0);
