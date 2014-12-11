#! /usr/bin/perl -w
# MD5: 89217e2bb98473d24894ab25004698fe
# TEST: ../rwsort/rwsort --fields=scc ../../tests/data.rwf | ./rwgroup --id-fields=scc | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwgroup = check_silk_app('rwgroup');
my $rwcat = check_silk_app('rwcat');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
$file{fake_cc} = get_data_or_exit77('fake_cc');
$ENV{SILK_COUNTRY_CODES} = "$SiLKTests::PWD/$file{fake_cc}";
my $cmd = "$rwsort --fields=scc $file{data} | $rwgroup --id-fields=scc | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "89217e2bb98473d24894ab25004698fe";

check_md5_output($md5, $cmd);
