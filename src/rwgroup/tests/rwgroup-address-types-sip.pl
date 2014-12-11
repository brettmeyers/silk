#! /usr/bin/perl -w
# MD5: 4abcbde5725c6a1b79ae1e01154ad5ad
# TEST: ../rwsort/rwsort --fields=stype ../../tests/data.rwf | ./rwgroup --id-fields=stype | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
$file{address_types} = get_data_or_exit77('address_types');
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwsort --fields=stype $file{data} | $rwgroup --id-fields=stype | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "4abcbde5725c6a1b79ae1e01154ad5ad";

check_md5_output($md5, $cmd);
