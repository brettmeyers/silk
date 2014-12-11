#! /usr/bin/perl -w
# MD5: 4230c82422d93f50cb973d6ea9eec1cd
# TEST: ../rwset/rwsetbuild /dev/null /tmp/rwscan-empty-input-emptyset && ./rwscan --trw-sip-set=/tmp/rwscan-empty-input-emptyset ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwscan = check_silk_app('rwscan');
my $rwsetbuild = check_silk_app('rwsetbuild');
my %file;
$file{empty} = get_data_or_exit77('empty');
my %temp;
$temp{emptyset} = make_tempname('emptyset');
my $cmd = "$rwsetbuild /dev/null $temp{emptyset} && $rwscan --trw-sip-set=$temp{emptyset} $file{empty}";
my $md5 = "4230c82422d93f50cb973d6ea9eec1cd";

check_md5_output($md5, $cmd);
