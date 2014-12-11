#! /usr/bin/perl -w
# MD5: cb5efde20d8f8b51a852fff7ea6d0772
# TEST: ../rwsort/rwsort --fields=stime --reverse ../../tests/data.rwf | ./rwcount --load-scheme=1

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --fields=stime --reverse $file{data} | $rwcount --load-scheme=1";
my $md5 = "cb5efde20d8f8b51a852fff7ea6d0772";

check_md5_output($md5, $cmd);
