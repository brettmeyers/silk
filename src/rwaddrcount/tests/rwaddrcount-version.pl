#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwaddrcount --version

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my $cmd = "$rwaddrcount --version";

exit (check_exit_status($cmd) ? 0 : 1);
