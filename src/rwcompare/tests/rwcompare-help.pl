#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwcompare --help

use strict;
use SiLKTests;

my $rwcompare = check_silk_app('rwcompare');
my $cmd = "$rwcompare --help";

exit (check_exit_status($cmd) ? 0 : 1);
