#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwrtd2split --help

use strict;
use SiLKTests;

my $rwrtd2split = check_silk_app('rwrtd2split');
my $cmd = "$rwrtd2split --help";

exit (check_exit_status($cmd) ? 0 : 1);
