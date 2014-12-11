#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwscan --scan-mode=2 /dev/null

use strict;
use SiLKTests;

my $rwscan = check_silk_app('rwscan');
my $cmd = "$rwscan --scan-mode=2 /dev/null";

exit (check_exit_status($cmd) ? 0 : 1);
