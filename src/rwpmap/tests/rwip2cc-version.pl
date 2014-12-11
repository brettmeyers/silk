#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwip2cc --version

use strict;
use SiLKTests;

my $rwip2cc = check_silk_app('rwip2cc');
my $cmd = "$rwip2cc --version";

exit (check_exit_status($cmd) ? 0 : 1);
