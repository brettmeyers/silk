#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwswapbytes ../../tests/empty.rwf /dev/null

use strict;
use SiLKTests;

my $rwswapbytes = check_silk_app('rwswapbytes');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwswapbytes $file{empty} /dev/null";

exit (check_exit_status($cmd) ? 1 : 0);
