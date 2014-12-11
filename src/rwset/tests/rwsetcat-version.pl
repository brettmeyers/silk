#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsetcat --version

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my $cmd = "$rwsetcat --version";

exit (check_exit_status($cmd) ? 0 : 1);
