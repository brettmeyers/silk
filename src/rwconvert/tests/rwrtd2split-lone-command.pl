#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwrtd2split

use strict;
use SiLKTests;

my $rwrtd2split = check_silk_app('rwrtd2split');
my $cmd = "$rwrtd2split";

exit (check_exit_status($cmd) ? 1 : 0);
