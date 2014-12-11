#! /usr/bin/perl -w
# MD5: multiple
# TEST: ./rwsplit --basename=$temp --byte-limit=10000000 --seed=737292 --file-ratio=800 ../../tests/data.rwf && ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output $temp*

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $temp = make_tempname('byte-10M-file');
my $cmd = "$rwsplit --basename=$temp --byte-limit=10000000 --seed=737292 --file-ratio=800 $file{data}";

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
while (<DATA>) {
    my ($i, $md5) = split " ";
    my $f = sprintf("%s.%08d.rwf", $temp, $i);
    $cmd = "$rwcat --compression-method=none --byte-order=little --ipv4-output $f";
    check_md5_output($md5, $cmd);
}

__DATA__
403    0962d5acda1c4bcda54d72afd78f4658
1309   70be6147905f9e5e072eff7421c0e1e8
2363   dbd38943ba247aaa188e1a4f0dba2ad6
2592   8617324edaf677272499544e8ede11ae
3549   545b0fb382dd61d29c1c422bf8a30d62
4127   7759165866bf0da86b52d7a31c633d59
4854   da61a93c4d3e71ccd873dc2e9f8135fc
5863   d4feb09206534bcf49b4f646ff1b419c
6787   911df56a9d963da65b9399d4f7eb6ebb
7528   6fe06e39ba67e8f47a064f73cbef747a
8792   08e2bd84af2af456ff47844fef815a3e
9408   c03b6eea5cbc61415b0cd44d3d3cb92b
9761   e1117ba1f670587df2b22e04cea04f22
10752  24322618053dd22d75eeb260e906ee99
