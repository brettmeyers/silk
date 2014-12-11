#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwset --version

use strict;
use SiLKTests;

my $rwset = check_silk_app('rwset');
my $cmd = "$rwset --version";

exit (check_exit_status($cmd) ? 0 : 1);
