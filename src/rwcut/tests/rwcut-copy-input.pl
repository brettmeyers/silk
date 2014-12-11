#! /usr/bin/perl -w
# MD5: 371b9adb9156787a842a5a9661df4f92
# TEST: ./rwcut --fields=5 --output-path=/dev/null --copy-input=stdout ../../tests/data.rwf | ./rwcut --fields=5

use strict;
use SiLKTests;

my $rwcut = check_silk_app('rwcut');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcut --fields=5 --output-path=/dev/null --copy-input=stdout $file{data} | $rwcut --fields=5";
my $md5 = "371b9adb9156787a842a5a9661df4f92";

check_md5_output($md5, $cmd);
