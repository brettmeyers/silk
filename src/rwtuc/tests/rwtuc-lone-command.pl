#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwtuc

use strict;
use SiLKTests;

my $rwtuc = check_silk_app('rwtuc');

exit 77 if sub { return !(-t STDIN); }->();

my $cmd = "$rwtuc";

exit (check_exit_status($cmd) ? 1 : 0);
