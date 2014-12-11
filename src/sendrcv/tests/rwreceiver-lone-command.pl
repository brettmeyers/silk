#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwreceiver

use strict;
use SiLKTests;

my $rwreceiver = check_silk_app('rwreceiver');
my $cmd = "$rwreceiver";

exit (check_exit_status($cmd) ? 1 : 0);
