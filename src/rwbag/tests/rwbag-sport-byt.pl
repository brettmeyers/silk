#! /usr/bin/perl -w
# MD5: 809879d4e3d235a9e079613e8c9193fb
# TEST: ./rwbag --sport-bytes=- ../../tests/data.rwf | ./rwbagcat --key-format=decimal --delimited=, -

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --sport-bytes=- $file{data} | $rwbagcat --key-format=decimal --delimited=, -";
my $md5 = "809879d4e3d235a9e079613e8c9193fb";

check_md5_output($md5, $cmd);
