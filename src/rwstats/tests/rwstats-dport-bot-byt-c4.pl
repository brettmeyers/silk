#! /usr/bin/perl -w
# MD5: 056642fdbae32c33e6e1b4948721eb62
# TEST: ../rwfilter/rwfilter --dport=0-66,69-1023,8080 --pass=- ../../tests/data.rwf | ./rwstats --fields=dport --bottom --values=bytes --count=20

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --dport=0-66,69-1023,8080 --pass=- $file{data} | $rwstats --fields=dport --bottom --values=bytes --count=20";
my $md5 = "056642fdbae32c33e6e1b4948721eb62";

check_md5_output($md5, $cmd);
