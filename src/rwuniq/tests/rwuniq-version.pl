#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwuniq --version

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my $cmd = "$rwuniq --version";

exit (check_exit_status($cmd) ? 0 : 1);
