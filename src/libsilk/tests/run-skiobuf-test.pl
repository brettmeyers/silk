#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./skiobuf-test 2>&1

use strict;
use SiLKTests;

my $skiobuf_test = check_silk_app('skiobuf-test');
my $cmd = "$skiobuf_test 2>&1";

exit (check_exit_status($cmd) ? 0 : 1);
