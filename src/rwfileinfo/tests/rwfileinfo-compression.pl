#! /usr/bin/perl -w
# CMP_MD5
# TEST: ../rwcat/rwcat --compression-method=none ../../tests/empty.rwf | ./rwfileinfo --fields=compression --no-title -
# TEST: ./rwfileinfo --fields=4 --no-title ../../tests/empty.rwf

use strict;
use SiLKTests;

my $rwfileinfo = check_silk_app('rwfileinfo');
my $rwcat = check_silk_app('rwcat');
my %file;
$file{empty} = get_data_or_exit77('empty');

my $compr;
if ($SiLKTests::SK_ENABLE_OUTPUT_COMPRESSION =~ /LZO/) {
    $compr = 'lzo1x';
} elsif ($SiLKTests::SK_ENABLE_OUTPUT_COMPRESSION =~ /ZLIB/) {
    $compr = 'zlib';
} elsif ($SiLKTests::SK_ENABLE_OUTPUT_COMPRESSION =~ /NONE/) {
    $compr = 'none';
} else {
    exit 1;
}

my @cmds = ("$rwcat --compression-method=$compr $file{empty} | $rwfileinfo --fields=compression --no-title -",
            "$rwfileinfo --fields=4 --no-title $file{empty}");
my $md5_old;

for my $cmd (@cmds) {
    my $md5;
    compute_md5(\$md5, $cmd);
    if (!defined $md5_old) {
        $md5_old = $md5;
    }
    elsif ($md5_old ne $md5) {
        die "rwfileinfo-compression.pl: checksum mismatch [$md5] ($cmd)\n";
    }
}
