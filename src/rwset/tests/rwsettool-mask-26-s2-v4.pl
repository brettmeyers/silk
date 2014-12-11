#! /usr/bin/perl -w
# MD5: 86d7c2b20bf1e885bafb7242deac1827
# TEST: ./rwsettool --mask=26 ../../tests/set2-v4.set | ./rwsetcat

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set2} = get_data_or_exit77('v4set2');
my $cmd = "$rwsettool --mask=26 $file{v4set2} | $rwsetcat";
my $md5 = "86d7c2b20bf1e885bafb7242deac1827";

check_md5_output($md5, $cmd);
