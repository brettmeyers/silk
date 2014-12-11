#! /usr/bin/perl -w
# MD5: db1962ccbbdc28b58007bea886e2a371 OR 1cbcce6117416d87c73ce7bbc9905996
# TEST: ./rwcount ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwcount = check_silk_app('rwcount');
my %file;
$file{data} = get_data_or_exit77('data');
my $cmd = "$rwcount $file{data}";

# we may get different results depending on how the operations with
# double are computed.
my @md5_exp = ('db1962ccbbdc28b58007bea886e2a371',
               '1cbcce6117416d87c73ce7bbc9905996');

my $md5;
compute_md5(\$md5, $cmd);
for (@md5_exp) {
    if ($md5 eq $_) {
        exit 0;
    }
}
exit 1;
