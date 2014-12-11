#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwdedupe --help

use strict;
use SiLKTests;

my $rwdedupe = check_silk_app('rwdedupe');
my $cmd = "$rwdedupe --help";

exit (check_exit_status($cmd) ? 0 : 1);
