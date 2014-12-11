#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwpollexec

use strict;
use SiLKTests;

my $rwpollexec = check_silk_app('rwpollexec');
my $cmd = "$rwpollexec";

exit (check_exit_status($cmd) ? 1 : 0);
