#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwscan --version

use strict;
use SiLKTests;

my $rwscan = check_silk_app('rwscan');
my $cmd = "$rwscan --version";

exit (check_exit_status($cmd) ? 0 : 1);
