#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwdedupe --ignore-fields=1 </dev/null

use strict;
use SiLKTests;

my $rwdedupe = check_silk_app('rwdedupe');
my $cmd = "$rwdedupe --ignore-fields=1 </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
