#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwidsquery --version

use strict;
use SiLKTests;

my $rwidsquery = check_silk_app('rwidsquery');
my $cmd = "$rwidsquery --version";

exit (check_exit_status($cmd) ? 0 : 1);
