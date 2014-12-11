#! /usr/bin/perl -w
# MD5: multiple
# TEST: ./rwsplit --basename=$temp --byte-limit=10000000 --seed=737292 --sample-ratio=1000 ../../tests/data.rwf && ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output $temp*

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $temp = make_tempname('byte-10M-sample');
my $cmd = "$rwsplit --basename=$temp --byte-limit=10000000 --seed=737292 --sample-ratio=1000 $file{data}";

# clean up when we're done
END {
    if (!$ENV{SK_TESTS_SAVEOUTPUT}) {
        # remove files
        unlink glob($temp."*");
    }
}

if (!check_exit_status($cmd)) {
    exit 1;
}

# compute MD5 of each file
for (my $i = 0; my $md5 = <DATA>; ++$i) {
    chomp $md5;
    my $f = sprintf("%s.%08d.rwf", $temp, $i);
    $cmd = "$rwcat --compression-method=none --byte-order=little --ipv4-output $f";
    check_md5_output($md5, $cmd);
}

__DATA__
2335664cb266ae170eed727abbd12326
1538100f51bed192351a0243678012f1
a9fa60ff34bb31c4c29ed97e0ba69a0b
7a74aab73acc0b6ce341818840b4ff99
9bed61c9a80df8a060c98d14d7eddd27
d8d7a5014b2f522d0779a312c3943139
bfe0e9469f2ebbb5359f6833d63a9194
31d4e33023015e6b1005d98c7a622fe4
06ff34ff7abd044df9c3357b9e3c07a4
95e45c98621c5ea44e62ae93d4f3a1a9
abe239fd86ebd2f1089bc82647781114
ac668215f12d736631389f5d14c34b8d
8356285f8777cfe3faa1df6141188103
