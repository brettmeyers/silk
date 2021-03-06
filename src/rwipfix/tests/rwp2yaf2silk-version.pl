#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwp2yaf2silk --version

use strict;
use SiLKTests;

my $rwp2yaf2silk = check_silk_app('rwp2yaf2silk');
check_features(qw(ipfix));
my $cmd = "$rwp2yaf2silk --version";

exit (check_exit_status($cmd) ? 0 : 1);
