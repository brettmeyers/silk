#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwp2yaf2silk

use strict;
use SiLKTests;

my $rwp2yaf2silk = check_silk_app('rwp2yaf2silk');
check_features(qw(ipfix));

exit 77 if sub { return !(-t STDIN); }->();

my $cmd = "$rwp2yaf2silk";

exit (check_exit_status($cmd) ? 1 : 0);
