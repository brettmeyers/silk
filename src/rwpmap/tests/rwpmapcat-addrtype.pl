#! /usr/bin/perl -w
# MD5: 045c3a9fd792d585bec7047b42e2618b
# TEST: ./rwpmapcat --no-cidr --address-types=../../tests/address_types.pmap

use strict;
use SiLKTests;

my $rwpmapcat = check_silk_app('rwpmapcat');
my %file;
$file{address_types} = get_data_or_exit77('address_types');
my $cmd = "$rwpmapcat --no-cidr --address-types=$file{address_types}";
my $md5 = "045c3a9fd792d585bec7047b42e2618b";

check_md5_output($md5, $cmd);
