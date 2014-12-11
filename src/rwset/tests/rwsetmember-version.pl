#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsetmember --version

use strict;
use SiLKTests;

my $rwsetmember = check_silk_app('rwsetmember');
my $cmd = "$rwsetmember --version";

exit (check_exit_status($cmd) ? 0 : 1);
