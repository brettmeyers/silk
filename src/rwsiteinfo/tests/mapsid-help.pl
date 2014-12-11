#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./mapsid --help

use strict;
use SiLKTests;

my $mapsid = check_silk_app('mapsid');
my $cmd = "$mapsid --help";

exit (check_exit_status($cmd) ? 0 : 1);
