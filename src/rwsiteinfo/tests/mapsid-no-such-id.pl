#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./mapsid 9999

use strict;
use SiLKTests;

my $mapsid = check_silk_app('mapsid');
my $cmd = "$mapsid 9999";

exit (check_exit_status($cmd) ? 0 : 1);
