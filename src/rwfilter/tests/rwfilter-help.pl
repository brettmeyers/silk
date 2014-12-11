#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwfilter --help

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $cmd = "$rwfilter --help";

exit (check_exit_status($cmd) ? 0 : 1);
