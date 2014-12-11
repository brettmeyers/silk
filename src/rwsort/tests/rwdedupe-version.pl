#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwdedupe --version

use strict;
use SiLKTests;

my $rwdedupe = check_silk_app('rwdedupe');
my $cmd = "$rwdedupe --version";

exit (check_exit_status($cmd) ? 0 : 1);
