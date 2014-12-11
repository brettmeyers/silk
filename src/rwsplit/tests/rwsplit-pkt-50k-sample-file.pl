#! /usr/bin/perl -w
# MD5: multiple
# TEST: ./rwsplit --basename=$temp --packet-limit=50000 --seed=737292 --sample-ratio=20 --file-ratio=10 ../../tests/data.rwf && ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output $temp*

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $temp = make_tempname('pkt-50k-sample-file');
my $cmd = "$rwsplit --basename=$temp --packet-limit=50000 --seed=737292 --sample-ratio=20 --file-ratio=10 $file{data}";

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
9    6e0e8593a41713e09fc84012a41fffb3
18   2b845d4197a6fb3a36e7fec32c6899a8
22   c4dd7b50c01073e17ddebff401446c74
35   0d632aef0cc75ae59eef36311270790c
49   4ac21c38db8e15b3e081f9f8b839cee7
54   c6a466b3145b436247e3e1adba93d5af
67   c136fab015325d265140adece29f9964
79   70c421cf6c9fa7c1d000ad189963fa3c
81   8e0dbb81a2cc892e471024a9452da4d2
95   2cf5795be59b209deb3e81482340e879
108  9874bd756bdb77a3c2c98d5fc2aff7c4
113  6fa98851312277cb5d8ac69e6cfd609f
123  92c3832a953b969b42a706ec488c1be8
139  db76a5c2a12c551bee9dec10b2643690
143  865ec448dae859cd09e5fbffe2d7b5cc
