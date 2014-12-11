#! /usr/bin/perl -w
# MD5: 10c2ee2963d953f280162e5d337c3d9f
# TEST: ./rwcount --bin-size=604800 --load-scheme=0 --start-time=2009/02/10:00:00:00 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=604800 --load-scheme=0 --start-time=2009/02/10:00:00:00 $file{data}";
my $md5 = "10c2ee2963d953f280162e5d337c3d9f";

check_md5_output($md5, $cmd);
