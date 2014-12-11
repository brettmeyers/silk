#! /usr/bin/perl -w
# MD5: 8e31ab00f939dbce0b92a45b4a02f80e
# TEST: ./rwuniq --fields=stype,proto --values=packets --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
$file{address_types} = get_data_or_exit77('address_types');
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwuniq --fields=stype,proto --values=packets --sort-output $file{data}";
my $md5 = "8e31ab00f939dbce0b92a45b4a02f80e";

check_md5_output($md5, $cmd);
