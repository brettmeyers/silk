#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwdedupe

use strict;
use SiLKTests;

my $rwdedupe = check_silk_app('rwdedupe');
my $cmd = "$rwdedupe";

exit (check_exit_status($cmd) ? 1 : 0);
