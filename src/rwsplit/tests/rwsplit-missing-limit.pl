#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsplit --basename=/tmp/rwsplit-missing-limit-missing_limit ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my %file;
$file{empty} = get_data_or_exit77('empty');
my %temp;
$temp{missing_limit} = make_tempname('missing_limit');
my $cmd = "$rwsplit --basename=$temp{missing_limit} $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
