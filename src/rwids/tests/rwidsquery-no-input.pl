#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwidsquery --intype=fast

use strict;
use SiLKTests;

my $rwidsquery = check_silk_app('rwidsquery');
my $cmd = "$rwidsquery --intype=fast";

exit (check_exit_status($cmd) ? 1 : 0);
