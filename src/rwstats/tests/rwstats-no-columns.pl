#! /usr/bin/perl -w
# MD5: 89e097dff522627a43eaaaacb86f5519
# TEST: ./rwstats --fields=dip --count=10 --top --no-column --column-sep=, --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=dip --count=10 --top --no-column --column-sep=, --ipv6-policy=ignore $file{data}";
my $md5 = "89e097dff522627a43eaaaacb86f5519";

check_md5_output($md5, $cmd);
