#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwswapbytes --big-endian ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwswapbytes = check_silk_app('rwswapbytes');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwswapbytes --big-endian $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
