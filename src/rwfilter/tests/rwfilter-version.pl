#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwfilter --version

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my $cmd = "$rwfilter --version";

exit (check_exit_status($cmd) ? 0 : 1);
