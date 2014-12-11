#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwfilter --proto=1 ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwfilter --proto=1 $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
