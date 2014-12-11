#! /usr/bin/perl -w
# MD5: e4868a1db48d81165c100c11b73827af
# TEST: ./rwtotal --duration --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --duration --skip-zero $file{data}";
my $md5 = "e4868a1db48d81165c100c11b73827af";

check_md5_output($md5, $cmd);
