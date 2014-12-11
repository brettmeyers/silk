#! /usr/bin/perl -w
# MD5: bbe1ccae72895f2e22791ea049aba410
# TEST: ./rwsetcat --cidr ../../tests/set1-v4.set | ./rwsetbuild | ./rwsetcat --cidr

use strict;
use SiLKTests;

my $rwsetbuild = check_silk_app('rwsetbuild');
my $rwsetcat = check_silk_app('rwsetcat');
my %file;
$file{v4set1} = get_data_or_exit77('v4set1');
my $cmd = "$rwsetcat --cidr $file{v4set1} | $rwsetbuild | $rwsetcat --cidr";
my $md5 = "bbe1ccae72895f2e22791ea049aba410";

check_md5_output($md5, $cmd);
