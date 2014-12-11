#! /usr/bin/perl -w
# MD5: 497a3284488f0a97535373a522669f73
# TEST: ./rwstats --fields=sip --top --count=10 --output-path=/dev/null --copy-input=stdout ../../tests/data.rwf | ./rwstats --fields=dip --count=10 --ipv6-policy=ignore

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=sip --top --count=10 --output-path=/dev/null --copy-input=stdout $file{data} | $rwstats --fields=dip --count=10 --ipv6-policy=ignore";
my $md5 = "497a3284488f0a97535373a522669f73";

check_md5_output($md5, $cmd);
