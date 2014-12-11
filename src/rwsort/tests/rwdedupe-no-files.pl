#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwdedupe --ignore-fields=1

use strict;
use SiLKTests;

my $rwdedupe = check_silk_app('rwdedupe');
my $cmd = "$rwdedupe --ignore-fields=1";

exit (check_exit_status($cmd) ? 1 : 0);
