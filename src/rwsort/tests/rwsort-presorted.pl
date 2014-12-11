#! /usr/bin/perl -w
# MD5: 796448848fa25365cd3500772b9a9649
# TEST: ../rwfilter/rwfilter --proto=6 --pass=- --fail=/tmp/rwsort-presorted-data1b ../../tests/data.rwf | ./rwsort --field=9,1 --output-path=/tmp/rwsort-presorted-data1 && ../rwfilter/rwfilter --proto=17 --pass=- --fail=/tmp/rwsort-presorted-data2b /tmp/rwsort-presorted-data1b | ./rwsort --field=9,1 --output-path=/tmp/rwsort-presorted-data2 && ./rwsort --field=9,1 --output-path=/tmp/rwsort-presorted-data3 /tmp/rwsort-presorted-data2b && ./rwsort --field=9,1 --presorted /tmp/rwsort-presorted-data1 ../../tests/empty.rwf /tmp/rwsort-presorted-data2 ../../tests/empty.rwf /tmp/rwsort-presorted-data3 | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwfilter = check_silk_app('rwfilter');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
$file{empty} = get_data_or_exit77('empty');
my %temp;
$temp{data1} = make_tempname('data1');
$temp{data1b} = make_tempname('data1b');
$temp{data2} = make_tempname('data2');
$temp{data2b} = make_tempname('data2b');
$temp{data3} = make_tempname('data3');
my $cmd = "$rwfilter --proto=6 --pass=- --fail=$temp{data1b} $file{data} | $rwsort --field=9,1 --output-path=$temp{data1} && $rwfilter --proto=17 --pass=- --fail=$temp{data2b} $temp{data1b} | $rwsort --field=9,1 --output-path=$temp{data2} && $rwsort --field=9,1 --output-path=$temp{data3} $temp{data2b} && $rwsort --field=9,1 --presorted $temp{data1} $file{empty} $temp{data2} $file{empty} $temp{data3} | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "796448848fa25365cd3500772b9a9649";

check_md5_output($md5, $cmd);
