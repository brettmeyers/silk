#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwipfix2silk

use strict;
use SiLKTests;

my $rwipfix2silk = check_silk_app('rwipfix2silk');
check_features(qw(ipfix));

exit 77 if sub { return !(-t STDIN); }->();

my $cmd = "$rwipfix2silk";

exit (check_exit_status($cmd) ? 1 : 0);
