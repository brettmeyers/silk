#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./skdeque-test

use strict;
use SiLKTests;

my $skdeque_test = check_silk_app('skdeque-test');
my $cmd = "$skdeque_test";

exit (check_exit_status($cmd) ? 0 : 1);
