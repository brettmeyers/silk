#! /usr/bin/perl -w
# MD5: d41d8cd98f00b204e9800998ecf8427e
# TEST: ../rwcat/rwcat --byte-order=big --output-path=/tmp/rwcompare-big-big ../../tests/data.rwf && ./rwcompare ../../tests/data.rwf /tmp/rwcompare-big-big

use strict;
use SiLKTests;

my $rwcompare = check_silk_app('rwcompare');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my %temp;
$temp{big} = make_tempname('big');
my $cmd = "$rwcat --byte-order=big --output-path=$temp{big} $file{data} && $rwcompare $file{data} $temp{big}";
my $md5 = "d41d8cd98f00b204e9800998ecf8427e";

check_md5_output($md5, $cmd);
