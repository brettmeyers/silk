#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsetcat </dev/null

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $cmd = "$rwsetcat </dev/null";

exit (check_exit_status($cmd) ? 0 : 1);
