#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpollexec --help

use strict;
use SiLKTests;

my $rwpollexec = check_silk_app('rwpollexec');
my $cmd = "$rwpollexec --help";

exit (check_exit_status($cmd) ? 0 : 1);
