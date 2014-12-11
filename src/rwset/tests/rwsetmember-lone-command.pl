#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsetmember

use strict;
use SiLKTests;

my $rwsetmember = check_silk_app('rwsetmember');
my $cmd = "$rwsetmember";

exit (check_exit_status($cmd) ? 1 : 0);
