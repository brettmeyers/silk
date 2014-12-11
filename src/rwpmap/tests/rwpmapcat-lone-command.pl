#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwpmapcat

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my $cmd = "$rwpmapcat";

exit (check_exit_status($cmd) ? 1 : 0);
