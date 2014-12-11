#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsetcat

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');

exit 77 if sub { return !(-t STDIN); }->();

my $cmd = "$rwsetcat";

exit (check_exit_status($cmd) ? 1 : 0);
