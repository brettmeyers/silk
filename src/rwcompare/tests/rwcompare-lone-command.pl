#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwcompare

use strict;
use SiLKTests;

my $rwcompare = check_silk_app('rwcompare');
my $cmd = "$rwcompare";

exit (check_exit_status($cmd) ? 1 : 0);
