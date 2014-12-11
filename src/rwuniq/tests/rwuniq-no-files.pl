#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwuniq --fields=1

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my $cmd = "$rwuniq --fields=1";

exit (check_exit_status($cmd) ? 1 : 0);
