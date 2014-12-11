#! /usr/bin/perl -w
# ERR_MD5: ee3aa3f52235a167a0bd2c760edb7253
# TEST: ./rwpackchecker --value match-sport=123 --value match-dport=123 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwpackchecker = check_silk_app('rwpackchecker');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwpackchecker --value match-sport=123 --value match-dport=123 $file{data}";
my $md5 = "ee3aa3f52235a167a0bd2c760edb7253";

check_md5_output($md5, $cmd, 1);
