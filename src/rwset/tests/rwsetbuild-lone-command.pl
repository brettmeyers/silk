#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsetbuild

use strict;
use SiLKTests;

my $rwsetbuild = check_silk_app('rwsetbuild');

exit 77 if sub { return !(-t STDIN); }->();

my $cmd = "$rwsetbuild";

exit (check_exit_status($cmd) ? 1 : 0);
