#! /usr/bin/perl -w
# MD5: bdd37890531edcde4b24d890e7a3e0c9
# TEST: ./rwswapbytes --little-endian ../../tests/empty.rwf - | ../rwfileinfo/rwfileinfo --no-title --field=byte-order,count-records -

use strict;
use SiLKTests;

my $rwswapbytes = check_silk_app('rwswapbytes');
my $rwfileinfo = check_silk_app('rwfileinfo');
my %file;
$file{empty} = get_data_or_exit77('empty');
my $cmd = "$rwswapbytes --little-endian $file{empty} - | $rwfileinfo --no-title --field=byte-order,count-records -";
my $md5 = "bdd37890531edcde4b24d890e7a3e0c9";

check_md5_output($md5, $cmd);
