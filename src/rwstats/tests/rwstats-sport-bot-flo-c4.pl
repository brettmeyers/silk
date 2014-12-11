#! /usr/bin/perl -w
# MD5: 0e0d04411ae51f6b3f307d369a73f6c2
# TEST: ../rwfilter/rwfilter --sport=0-66,69-1023,8080 --pass=- ../../tests/data.rwf | ./rwstats --fields=sport --values=records --bottom --count=4

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --sport=0-66,69-1023,8080 --pass=- $file{data} | $rwstats --fields=sport --values=records --bottom --count=4";
my $md5 = "0e0d04411ae51f6b3f307d369a73f6c2";

check_md5_output($md5, $cmd);
