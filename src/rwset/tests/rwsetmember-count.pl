#! /usr/bin/perl -w
# MD5: 4e1b080408698c01f9f7b3bf0568cbda
# TEST: ./rwset --sip-file=stdout ../../tests/data.rwf | ./rwsetmember --count 192.168.x.x -

use strict;
use SiLKTests;

my $rwsetmember = check_silk_app('rwsetmember');
my $rwset = check_silk_app('rwset');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwset --sip-file=stdout $file{data} | $rwsetmember --count 192.168.x.x -";
my $md5 = "4e1b080408698c01f9f7b3bf0568cbda";

check_md5_output($md5, $cmd);
