#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwcombine --version

use strict;
use SiLKTests;

my $rwcombine = check_silk_app('rwcombine');
my $cmd = "$rwcombine --version";

exit (check_exit_status($cmd) ? 0 : 1);
