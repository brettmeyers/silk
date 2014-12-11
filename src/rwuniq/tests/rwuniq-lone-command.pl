#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwuniq

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my $cmd = "$rwuniq";

exit (check_exit_status($cmd) ? 1 : 0);
