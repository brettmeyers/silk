#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwidsquery

use strict;
use SiLKTests;

my $rwidsquery = check_silk_app('rwidsquery');
my $cmd = "$rwidsquery";

exit (check_exit_status($cmd) ? 1 : 0);
