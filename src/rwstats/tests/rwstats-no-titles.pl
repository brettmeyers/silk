#! /usr/bin/perl -w
# MD5: 9e78cf14542764153a1176bcc6b7508e
# TEST: ./rwstats --fields=dip --count=10 --top --no-titles --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=dip --count=10 --top --no-titles --ipv6-policy=ignore $file{data}";
my $md5 = "9e78cf14542764153a1176bcc6b7508e";

check_md5_output($md5, $cmd);
