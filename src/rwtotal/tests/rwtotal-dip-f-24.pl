#! /usr/bin/perl -w
# MD5: ef5cb14267c29503e7a94427c626c104
# TEST: ./rwtotal --dip-first-24 --skip-zero ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwtotal = check_silk_app('rwtotal');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwtotal --dip-first-24 --skip-zero $file{data}";
my $md5 = "ef5cb14267c29503e7a94427c626c104";

check_md5_output($md5, $cmd);
