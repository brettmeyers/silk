#! /usr/bin/perl -w
# STATUS: ERR
# TEST: ./rwsplit --basename=/tmp/rwsplit-multiple-limit-multiple_limit --ip-limit=200 --flow-limit=900 ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my %file;
$file{empty} = get_data_or_exit77('empty');
my %temp;
$temp{multiple_limit} = make_tempname('multiple_limit');
my $cmd = "$rwsplit --basename=$temp{multiple_limit} --ip-limit=200 --flow-limit=900 $file{empty}";

exit (check_exit_status($cmd) ? 1 : 0);
