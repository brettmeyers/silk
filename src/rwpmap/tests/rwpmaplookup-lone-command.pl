#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwpmaplookup

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my $cmd = "$rwpmaplookup";

exit (check_exit_status($cmd) ? 1 : 0);
