#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpmaplookup --version

use strict;
use SiLKTests;

my $rwpmaplookup = check_silk_app('rwpmaplookup');
my $cmd = "$rwpmaplookup --version";

exit (check_exit_status($cmd) ? 0 : 1);
