#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwip2cc </dev/null

use strict;
use SiLKTests;

my $rwip2cc = check_silk_app('rwip2cc');
my $cmd = "$rwip2cc </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
