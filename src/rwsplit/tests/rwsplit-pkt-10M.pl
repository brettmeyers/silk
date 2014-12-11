#! /usr/bin/perl -w
# MD5: multiple
# TEST: ./rwsplit --basename=$temp --packet-limit=10000000 ../../tests/data.rwf && ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output $temp*

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $temp = make_tempname('pkt-10M');
my $cmd = "$rwsplit --basename=$temp --packet-limit=10000000 $file{data}";

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
bb2bf9103777d2809c53b68ec25d7c04
d8c42486b6e5574392e16b540c41714a
e19861075eed7ccca2f9b06e4f694b78
e2372f7687eadc876e6d68b132897081
56a1f5404b101e1636ecfea5cd9285ae
4229731aa6c1f61de3a41f1f5f39b556
23a7bcb679a41279661ee129409aa4db
ddaa0b4c3a0c6a22d03aa24d38e4c8dd
e753537b84aae63a1863e0066558df7d
4ffd496416d5d7e3a2d3cbb69790cc36
14ed125fbfefbcb43b0ca3636a801fb5
95b7a68df73bb5fc36bfa685cf7c69f8
1162a5e12ae8619aa370f6438807dcf5
fc83bea5dab164fb7352eba21ea7bfbf
cfab18d9ce60e75e95e705e5ea065551
