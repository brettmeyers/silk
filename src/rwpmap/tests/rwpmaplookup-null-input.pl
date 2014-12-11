#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwpmaplookup </dev/null

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my $cmd = "$rwpmaplookup </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
