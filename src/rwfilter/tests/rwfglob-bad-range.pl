#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwfglob --data-rootdir=. --print-missing --start-date=2009/02/12:16 --end-date=2009/02/12:14

use strict;
use SiLKTests;

my $rwfglob = check_silk_app('rwfglob');
my $cmd = "$rwfglob --data-rootdir=. --print-missing --start-date=2009/02/12:16 --end-date=2009/02/12:14";

exit (check_exit_status($cmd) ? 1 : 0);
