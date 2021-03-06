#! /usr/bin/perl -w
# MD5: 796448848fa25365cd3500772b9a9649
# TEST: cat ../../tests/data.rwf | ./rwsort --field=9,1 --input-pipe=stdin | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "cat $file{data} | $rwsort --field=9,1 --input-pipe=stdin | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "796448848fa25365cd3500772b9a9649";

check_md5_output($md5, $cmd);
