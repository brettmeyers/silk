#! /usr/bin/perl -w
# MD5: 95bf1beb48012e76be27410c6b250356
# TEST: ./rwsettool --mask=23 ../../tests/set2-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --mask=23 $file{v4set2} | $rwsetcat";
my $md5 = "95bf1beb48012e76be27410c6b250356";

check_md5_output($md5, $cmd);
