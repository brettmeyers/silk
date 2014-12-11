#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwallformats

use strict;
use SiLKTests;

my $rwallformats = check_silk_app('rwallformats');
my $cmd = "$rwallformats";

exit (check_exit_status($cmd) ? 1 : 0);
