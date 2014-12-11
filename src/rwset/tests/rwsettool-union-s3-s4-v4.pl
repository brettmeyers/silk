#! /usr/bin/perl -w
# MD5: 27b5cd66f4784fcee67e557660c5e916
# TEST: ./rwsettool --union ../../tests/set3-v4.set ../../tests/set4-v4.set | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsettool = check_silk_app('rwsettool');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set3} = get_data_or_exit77('v4set3');
$file{v4set4} = get_data_or_exit77('v4set4');
my $cmd = "$rwsettool --union $file{v4set3} $file{v4set4} | $rwsetcat --cidr";
my $md5 = "27b5cd66f4784fcee67e557660c5e916";

check_md5_output($md5, $cmd);
