#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./mapsid --version

use strict;
use SiLKTests;

my $mapsid = check_silk_app('mapsid');
my $cmd = "$mapsid --version";

exit (check_exit_status($cmd) ? 0 : 1);
