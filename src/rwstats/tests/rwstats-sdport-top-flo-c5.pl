#! /usr/bin/perl -w
# MD5: b8064d7870a4e3077cb84b61865b4979
# TEST: ../rwfilter/rwfilter --type=in,inweb --pass=stdout ../../tests/data.rwf | ./rwstats --fields=sport,dport --count=5

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my $rwfilter = check_silk_app('rwfilter');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwfilter --type=in,inweb --pass=stdout $file{data} | $rwstats --fields=sport,dport --count=5";
my $md5 = "b8064d7870a4e3077cb84b61865b4979";

check_md5_output($md5, $cmd);
