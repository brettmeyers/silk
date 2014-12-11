#! /usr/bin/perl -w
# CMP_MD5
# TEST: ../rwcat/rwcat --byte-order=little ../../tests/empty.rwf | ./rwfileinfo --fields=byte-order --no-title -
# TEST: ./rwfileinfo --fields=3 --no-title ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwfileinfo = check_silk_app('rwfileinfo');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{empty} = get_data_or_exit77('empty');

my $order = ($SiLKTests::IS_BIG_ENDIAN ? "big" : "little");
my @cmds = ("$rwcat --byte-order=$order $file{empty} | $rwfileinfo --fields=byte-order --no-title -",
            "$rwfileinfo --fields=3 --no-title $file{empty}");
my $md5_old;

for my $cmd (@cmds) {
    my $md5;
    compute_md5(\$md5, $cmd);
    if (!defined $md5_old) {
        $md5_old = $md5;
    }
    elsif ($md5_old ne $md5) {
        die "rwfileinfo-byte-order.pl: checksum mismatch [$md5] ($cmd)\n";
    }
}
