#! /usr/bin/perl -w
# MD5: cf89a898adcf8c160f270351dfd0469a
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ../rwsort/rwsort --fields=3-5 --output-path=/tmp/rwuniq-ports-proto-multi-pre-in && ../rwfilter/rwfilter --type=in,inweb --fail=stdout ../../tests/data.rwf | ../rwsort/rwsort --fields=3-5 --output-path=/tmp/rwuniq-ports-proto-multi-pre-out && ./rwuniq --fields=3-5 --presorted-input --no-title /tmp/rwuniq-ports-proto-multi-pre-in /tmp/rwuniq-ports-proto-multi-pre-out

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my $rwfilter = check_silk_app('rwfilter');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my %temp;
$temp{in} = make_tempname('in');
$temp{out} = make_tempname('out');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwsort --fields=3-5 --output-path=$temp{in} && $rwfilter --type=in,inweb --fail=stdout $file{data} | $rwsort --fields=3-5 --output-path=$temp{out} && $rwuniq --fields=3-5 --presorted-input --no-title $temp{in} $temp{out}";
my $md5 = "cf89a898adcf8c160f270351dfd0469a";

check_md5_output($md5, $cmd);
