#! /usr/bin/perl -w
# MD5: df90969385ba8e7ec1a2bbe4f389f194
# TEST: ./rwfileinfo --fields=command-lines,version ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwfileinfo = check_silk_app('rwfileinfo');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfileinfo --fields=command-lines,version $file{data}";
my $md5 = "df90969385ba8e7ec1a2bbe4f389f194";

check_md5_output($md5, $cmd);
