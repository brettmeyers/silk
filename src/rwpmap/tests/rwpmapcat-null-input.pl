#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwpmapcat </dev/null

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my $cmd = "$rwpmapcat </dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
