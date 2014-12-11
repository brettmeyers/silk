#! /usr/bin/perl -w
# MD5: e7dda1e25cecd550585fe7e272ab0350
# TEST: ./rwcount --bin-size=3600 --load-scheme=0 --start-epoch=2009/02/12T20:00:00 --end-epoch=2009/02/13T20:00:00 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount --bin-size=3600 --load-scheme=0 --start-epoch=2009/02/12T20:00:00 --end-epoch=2009/02/13T20:00:00 $file{data}";
my $md5 = "e7dda1e25cecd550585fe7e272ab0350";

check_md5_output($md5, $cmd);
