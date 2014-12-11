#! /usr/bin/perl -w
# STATUS: OK
# TEST: ./rwpmapcat --version

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my $cmd = "$rwpmapcat --version";

exit (check_exit_status($cmd) ? 0 : 1);
