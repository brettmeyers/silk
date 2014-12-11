#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpmapcat --help

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my $cmd = "$rwpmapcat --help";

exit (check_exit_status($cmd) ? 0 : 1);
