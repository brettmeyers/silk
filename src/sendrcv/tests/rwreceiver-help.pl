#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwreceiver --help

use strict;
use SiLKTests;

my $rwreceiver = check_silk_app('rwreceiver');
my $cmd = "$rwreceiver --help";

exit (check_exit_status($cmd) ? 0 : 1);
