#! /usr/bin/perl -w
# MD5: 9404b5e522dab2e76f2e523b6b7314d4
# TEST: ../rwsort/rwsort --fields=dcc ../../tests/data.rwf | ./rwgroup --id-fields=dcc | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
my $cmd = "$rwsort --fields=dcc $file{data} | $rwgroup --id-fields=dcc | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "9404b5e522dab2e76f2e523b6b7314d4";

check_md5_output($md5, $cmd);
