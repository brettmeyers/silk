#! /usr/bin/perl -w
# MD5: 93858cb8293bb0de0cfd33b5339262ce
# TEST: ../rwsort/rwsort --fields=dtype ../../tests/data.rwf | ./rwgroup --id-fields=dtype | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
$file{address_types} = get_data_or_exit77('address_types');
$ENV{SILK_ADDRESS_TYPES} = "$SiLKTests::PWD/$file{address_types}";
my $cmd = "$rwsort --fields=dtype $file{data} | $rwgroup --id-fields=dtype | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "93858cb8293bb0de0cfd33b5339262ce";

check_md5_output($md5, $cmd);
