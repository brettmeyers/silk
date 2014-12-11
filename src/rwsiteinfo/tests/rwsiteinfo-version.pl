#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwsiteinfo --version

use strict;
use SiLKTests;

my $rwsiteinfo = check_silk_app('rwsiteinfo');
my $cmd = "$rwsiteinfo --version";

exit (check_exit_status($cmd) ? 0 : 1);
