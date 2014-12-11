#! /usr/bin/perl -w
# MD5: 3eca15c9293685664b37fc94bfe9db5f
# TEST: ./rwsettool --difference ../../tests/set4-v4.set ../../tests/set3-v4.set | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set3} = get_data_or_exit77('v4set3');
$file{v4set4} = get_data_or_exit77('v4set4');
my $cmd = "$rwsettool --difference $file{v4set4} $file{v4set3} | $rwsetcat --cidr";
my $md5 = "3eca15c9293685664b37fc94bfe9db5f";

check_md5_output($md5, $cmd);
