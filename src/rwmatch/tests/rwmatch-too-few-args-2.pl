#! /usr/bin/perl -w
# ERR_MD5: 667cebd0f7fff06ccdfdc8448d0eed2a
# TEST: ./rwmatch --relate=1,2 ../../tests/data.rwf ../../tests/data.rwf 2>&1

use strict;
use SiLKTests;

my $rwmatch = check_silk_app('rwmatch');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwmatch --relate=1,2 $file{data} $file{data} 2>&1";
my $md5 = "667cebd0f7fff06ccdfdc8448d0eed2a";

check_md5_output($md5, $cmd, 1);
