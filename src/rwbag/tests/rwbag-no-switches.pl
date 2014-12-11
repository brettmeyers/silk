#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwbag ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwbag $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
