#! /usr/bin/perl -w
# ERR_MD5: abf21fd05001c18a7c70916805039976
# TEST: ./rwmatch --relate=1,2 ../../tests/data.rwf 2>&1

use strict;
use SiLKTests;

my $rwmatch = check_silk_app('rwmatch');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwmatch --relate=1,2 $file{data} 2>&1";
my $md5 = "abf21fd05001c18a7c70916805039976";

check_md5_output($md5, $cmd, 1);
