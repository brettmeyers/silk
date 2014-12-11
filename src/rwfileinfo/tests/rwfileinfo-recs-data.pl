#! /usr/bin/perl -w
# MD5: 3c24448fd22dd8096c047f63448d39ff
# TEST: ./rwfileinfo --fields=count-records ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwfileinfo = check_silk_app('rwfileinfo');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfileinfo --fields=count-records $file{data}";
my $md5 = "3c24448fd22dd8096c047f63448d39ff";

check_md5_output($md5, $cmd);
