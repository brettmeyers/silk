#! /usr/bin/perl -w
# MD5: a68e8d7fc7ae52c6b19655bb743dda81
# TEST: ./rwsort --fields=dcc ../../tests/data.rwf | ../rwuniq/rwuniq --fields=dcc --values=dip-distinct --ipv6=ignore --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
my $cmd = "$rwsort --fields=dcc $file{data} | $rwuniq --fields=dcc --values=dip-distinct --ipv6=ignore --presorted-input";
my $md5 = "a68e8d7fc7ae52c6b19655bb743dda81";

check_md5_output($md5, $cmd);
