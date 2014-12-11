#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsplit --version

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my $cmd = "$rwsplit --version";

exit (check_exit_status($cmd) ? 0 : 1);
