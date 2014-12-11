#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwipfix2silk --version

use strict;
use SiLKTests;

my $rwipfix2silk = check_silk_app('rwipfix2silk');
check_features(qw(ipfix));
my $cmd = "$rwipfix2silk --version";

exit (check_exit_status($cmd) ? 0 : 1);
