#! /usr/bin/perl -w
# MD5: multiple
# TEST: ./rwsplit --basename=$temp --flow-limit=10000 ../../tests/data.rwf && ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output $temp*

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $temp = make_tempname('flow-10k');
my $cmd = "$rwsplit --basename=$temp --flow-limit=10000 $file{data}";

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
66be9b4031cc91dca0768869cb056219
2a98ff8475cb7dff4b7a66b61ad12061
e8f24a56c7ee60713b4ae4a5b9952f6e
2a566335ce52a902c68e4a2ed6f509cd
69aa396656bce81597d33c688669211b
61717ef7a7ce09bbac4dca8d3061902f
b215c9d2ab7c3e940080fd0a04944157
df8eea32e5738f96548be7c725009224
ec684d42acd24d86ae1999b15a25b257
f6a5b4284afbae51fed70d3a4a814f99
c14b6f1913adbc853f933cf548853db1
f1eea1427f86e01489d3945a06e9a9ac
53649f08e9ef0766ed6ed16881136786
701dd74552ac06d63ea3e8525fa55b33
da5308e4b7889a6bdb9b1f5b3424543f
7d5377c346448ac2cf34eb847fa07064
0d2cd8ebec3b04d676e2ded62e5345f5
48089614988d9e0fd67e21cd0b1a3ec9
11d707133e9465b4ed687d97f3c42464
ae273912da0b614ee445cbc807ee6430
94938461765a51e2a658ed2827140e60
fdc54341ae37fdd256d9c6be158bb881
971ab1ae8d2b249d0d6f2f6cf4d08cfa
7a502aced8caed4a0245a00d48f4ced0
67b74b7394fc383c78d4dc60eca90f62
5d5e7e8108183dc9c7f4fb3cda813355
b7dbb63ac7b678c2b7a4b2dca005d821
b9ff81c31fd62e133ac16139b986b8ea
d167e492de490344cc1903400fe4fc07
4bcb50fc8de993352ede5e9b73bf6cee
c1a07e097bd948b1c836a0fe05442a82
9962c9b03e9416df6e20ab353ee7bc83
92c6360796f4ff5418ac242243b84e4a
fae0dd0a67baea3f3e8acb635d694c4f
c332aef7f8fda5c0063c92438d7c4812
bc03bc180327c2026c6748646e19b008
c96781fd30674f6f4a3c059c0a3635b4
42a269702236fb882fec861bf8b1ad95
a9c5ab0828af95a6946b96544cd86299
ae0bcdf7e7a48f2aa893d780a9687ce2
69bd28ae2dc679790ae87d3c364a5b55
3a43a9205644b345bda24465fc37d198
1db02425c3521634f59635c243e2c53d
15aee7801004ba1c11c2c51e590ca497
5287d1432fe71dfff6a6a188c5441ba8
c755717b9bbc9ce833103facea0f281d
5e542671f21b66fd6f15bc43c92fb149
99dc2f724f2e5fee80d8d2310563c038
ea2a092348005d3015cbd4c4b4df5f13
4115be309371dbdf4e629794be1722d7
16e20ae77b7dd4a9e2fcd4d2cbccb551
