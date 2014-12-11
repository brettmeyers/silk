#! /usr/bin/perl -w
# MD5: 07ebe40111c431517a65dd08656976e0
# TEST: ./rwcount --bin-size=3600 --load-scheme=1 --no-columns --no-title ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=3600 --load-scheme=1 --no-columns --no-title $file{data}";
my $md5 = "07ebe40111c431517a65dd08656976e0";

check_md5_output($md5, $cmd);
