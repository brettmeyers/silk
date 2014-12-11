#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwtuc --help

use strict;
use SiLKTests;

my $rwtuc = check_silk_app('rwtuc');
my $cmd = "$rwtuc --help";

exit (check_exit_status($cmd) ? 0 : 1);
