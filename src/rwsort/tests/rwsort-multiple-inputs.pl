#! /usr/bin/perl -w
# MD5: 5b9c0de6640589e6822fd2d08bbe3f65
# TEST: ./rwsort --field=9,1 ../../tests/data.rwf ../../tests/data.rwf | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwsort --field=9,1 $file{data} $file{data} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "5b9c0de6640589e6822fd2d08bbe3f65";

check_md5_output($md5, $cmd);
