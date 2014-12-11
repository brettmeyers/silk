#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwreceiver --version

use strict;
use SiLKTests;

my $rwreceiver = check_silk_app('rwreceiver');
my $cmd = "$rwreceiver --version";

exit (check_exit_status($cmd) ? 0 : 1);
