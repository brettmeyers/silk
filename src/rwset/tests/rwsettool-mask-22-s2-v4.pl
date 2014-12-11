#! /usr/bin/perl -w
# MD5: 0aa59b97ba1159248aead1e401a3583d
# TEST: ./rwsettool --mask=22 ../../tests/set2-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --mask=22 $file{v4set2} | $rwsetcat";
my $md5 = "0aa59b97ba1159248aead1e401a3583d";

check_md5_output($md5, $cmd);
