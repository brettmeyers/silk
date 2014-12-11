#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwuniq ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwuniq $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
