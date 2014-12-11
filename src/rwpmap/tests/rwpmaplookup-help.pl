#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpmaplookup --help

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my $cmd = "$rwpmaplookup --help";

exit (check_exit_status($cmd) ? 0 : 1);
