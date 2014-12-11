#! /usr/bin/perl -w
# MD5: a3ff9dd764d7f3a206d011467d8a679d
# TEST: ./rwstats --fields=sip --values=bytes --count=100 --top --ipv6-policy=ignore ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwstats --fields=sip --values=bytes --count=100 --top --ipv6-policy=ignore $file{data}";
my $md5 = "a3ff9dd764d7f3a206d011467d8a679d";

check_md5_output($md5, $cmd);
