#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwtuc /dev/null

use strict;
use SiLKTests;

my $rwtuc = check_silk_app('rwtuc');
my $cmd = "$rwtuc /dev/null";

exit (check_exit_status($cmd) ? 0 : 1);
