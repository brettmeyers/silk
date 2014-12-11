#! /usr/bin/perl -w
# MD5: ba246774d4ea7411730606ca6747059a
# TEST: ./rwsetcat --count-ips ../../tests/set1-v4.set

use strict;
use SiLKTests;

my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsetcat --count-ips $file{v4set1}";
my $md5 = "ba246774d4ea7411730606ca6747059a";

check_md5_output($md5, $cmd);
