#! /usr/bin/perl -w
# MD5: multiple
# TEST: ./rwsplit --basename=$temp --byte-limit=10000000 --max-outputs=4 ../../tests/data.rwf && ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output $temp*

use strict;
use SiLKTests;

my $rwsplit = check_silk_app('rwsplit');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{data} = get_data_or_exit77('data');
my $temp = make_tempname('byte-10M-max');
my $cmd = "$rwsplit --basename=$temp --byte-limit=10000000 --max-outputs=4 $file{data}";

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
17f021f501240f1b3699d16c630ee1df
6bbbc694984523be4a56f41d3a5c4a03
728503e6d9c7cd1c6e5a87702226e3d1
8ede72a93de49a1a87853ccfb249beaf
