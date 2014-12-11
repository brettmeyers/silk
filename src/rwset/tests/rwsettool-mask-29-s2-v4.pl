#! /usr/bin/perl -w
# MD5: 5ad32dff1a29902156400024b17aef3b
# TEST: ./rwsettool --mask=29 ../../tests/set2-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --mask=29 $file{v4set2} | $rwsetcat";
my $md5 = "5ad32dff1a29902156400024b17aef3b";

check_md5_output($md5, $cmd);
