#! /usr/bin/perl -w
# MD5: 930654107271efc09aefc99fd57eee7d
# TEST: ./rwsort --fields=dtype ../../tests/data.rwf | ../rwuniq/rwuniq --fields=dtype --values=dip-distinct --delimited --ipv6=ignore --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{address_types} = get_data_or_exit77('address_types');
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwsort --fields=dtype $file{data} | $rwuniq --fields=dtype --values=dip-distinct --delimited --ipv6=ignore --presorted-input";
my $md5 = "930654107271efc09aefc99fd57eee7d";

check_md5_output($md5, $cmd);
