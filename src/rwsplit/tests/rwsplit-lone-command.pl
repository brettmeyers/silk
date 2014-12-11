#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsplit

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my $cmd = "$rwsplit";

exit (check_exit_status($cmd) ? 1 : 0);
