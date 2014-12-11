#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwset

use strict;
use SiLKTests;

my $rwset = check_silk_app('rwset');
my $cmd = "$rwset";

exit (check_exit_status($cmd) ? 1 : 0);
