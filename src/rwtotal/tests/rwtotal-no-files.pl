#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwtotal --sport

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my $cmd = "$rwtotal --sport";

exit (check_exit_status($cmd) ? 1 : 0);
