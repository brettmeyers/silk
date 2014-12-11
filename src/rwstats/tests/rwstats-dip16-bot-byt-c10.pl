#! /usr/bin/perl -w
# MD5: 1035013a65322c1fbf7a166e70c3fc06
# TEST: ./rwstats --dip=16 --values=bytes --count=10 --bottom --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --dip=16 --values=bytes --count=10 --bottom --ipv6-policy=ignore $file{data}";
my $md5 = "1035013a65322c1fbf7a166e70c3fc06";

check_md5_output($md5, $cmd);
