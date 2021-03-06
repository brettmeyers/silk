#! /usr/bin/perl -w
# MD5: b826093d399f36fd574df06e80084f7c
# TEST: ./rwstats --fields=dport --values=dip-distinct,records --threshold=5000 --no-percent ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=dport --values=dip-distinct,records --threshold=5000 --no-percent $file{data}";
my $md5 = "b826093d399f36fd574df06e80084f7c";

check_md5_output($md5, $cmd);
