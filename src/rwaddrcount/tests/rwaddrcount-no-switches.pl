#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwaddrcount ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwaddrcount = check_silk_app('rwaddrcount');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwaddrcount $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
