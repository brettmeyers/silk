#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwcount --version

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my $cmd = "$rwcount --version";

exit (check_exit_status($cmd) ? 0 : 1);
