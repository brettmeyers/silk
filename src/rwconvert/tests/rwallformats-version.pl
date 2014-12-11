#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwallformats --version

use strict;
use SiLKTests;

my $rwallformats = check_silk_app('rwallformats');
my $cmd = "$rwallformats --version";

exit (check_exit_status($cmd) ? 0 : 1);
