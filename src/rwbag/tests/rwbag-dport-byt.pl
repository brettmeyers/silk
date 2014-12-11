#! /usr/bin/perl -w
# MD5: 0515ec831f02e3e42d63a245b99e8f2a
# TEST: ./rwbag --dport-bytes=- ../../tests/data.rwf | ./rwbagcat --key-format=decimal --no-final-delimiter -

use strict;
use SiLKTests;

my $rwbag = check_silk_app('rwbag');
my $rwbagcat = check_silk_app('rwbagcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwbag --dport-bytes=- $file{data} | $rwbagcat --key-format=decimal --no-final-delimiter -";
my $md5 = "0515ec831f02e3e42d63a245b99e8f2a";

check_md5_output($md5, $cmd);
