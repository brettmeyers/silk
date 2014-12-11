#! /usr/bin/perl -w
# MD5: 7429f78964a152d9e8250c07202cec56
# TEST: ./rwuniq --fields=stime,proto --bin-time=86400 --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwuniq --fields=stime,proto --bin-time=86400 --sort-output $file{data}";
my $md5 = "7429f78964a152d9e8250c07202cec56";

check_md5_output($md5, $cmd);
