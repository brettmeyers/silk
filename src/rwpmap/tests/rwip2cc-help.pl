#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwip2cc --help

use strict;
use SiLKTests;

my $rwip2cc = check_silk_app('rwip2cc');
my $cmd = "$rwip2cc --help";

exit (check_exit_status($cmd) ? 0 : 1);
