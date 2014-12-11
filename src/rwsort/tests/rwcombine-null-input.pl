#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwcombine --ignore-fields=1 </dev/null

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my $cmd = "$rwcombine --ignore-fields=1 </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
