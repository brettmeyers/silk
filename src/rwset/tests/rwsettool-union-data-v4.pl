#! /usr/bin/perl -w
# MD5: multiple
# TEST: multiple
# RCSIDENT("$SiLK: rwsettool-union-data-v4.pl 6ed5a5c04fbe 2013-01-31 20:17:06Z mthomas $")

use strict;
use SiLKTests;

my $NAME = $0;
$NAME =~ s,.*/,,;

# find the apps we need.  this will exit 77 if they're not available
my $rwset = check_silk_app('rwset');
my $rwsetbuild = check_silk_app('rwsetbuild');
my $rwsetcat = check_silk_app('rwsetcat');
my $rwsettool = check_silk_app('rwsettool');
my $rwsplit = check_silk_app('rwsplit');

# switches for every rwset and rwsettool invocation
my $common_args = '--compression=none --invocation-strip';

# find the data files we use as sources, or exit 77
my %file;
$file{data} = get_data_or_exit77('data');

# create our tempdir
my $tmpdir = make_tempdir();

# basename for files that get created in $tmpdir
my $basename = 'v4';

# result of running MD5 on /dev/null
my $empty_md5 = 'd41d8cd98f00b204e9800998ecf8427e';

# result of running MD5 on "echo 0"
my $zero_md5 = '897316929176464ebc9ad085f31e7284';

my ($cmd, $md5);

#### CREATE FLOW FILES

$cmd = "$rwsplit --basename=$tmpdir/$basename --flow-limit=5000 $file{data}";
check_md5_output($empty_md5, $cmd);


#### CREATE AND UNION THE IPSETS

# create initial empty IPsets
my %sets = (
    sip => "$tmpdir/$basename-sip.set",
    dip => "$tmpdir/$basename-dip.set",
    any => "$tmpdir/$basename-any.set",
    );
for my $s (keys %sets) {
    $cmd = "cat /dev/null | $rwsetbuild $common_args - $sets{$s}";
    check_md5_output($empty_md5, $cmd);
}


# loop over the flow files
opendir T, $tmpdir
    or die "$NAME: Cannot opendir($tmpdir): $!\n";
for my $f (readdir T) {
    next unless $f =~ /^$basename\.\d+\.rwf$/;

    my %oldsets = %sets;

    for my $s (keys %sets) {
        # this is the new output file
        $sets{$s} = "$tmpdir/$f.$s.union";

        # create an IPset and union it with the rollup of all IPsets
        $cmd = "$rwset $common_args --$s=- $tmpdir/$f"
            ." | $rwsettool $common_args --output-path=$sets{$s}"
            ." --union - $oldsets{$s}";
        check_md5_output($empty_md5, $cmd);

        # remove the new set from the old set, giving an empty set
        $cmd = "$rwsettool $common_args --difference $oldsets{$s} $sets{$s}"
            ." | $rwsetcat --count";
        check_md5_output($zero_md5, $cmd);

        # intersect the new set with the old set; result should be
        # identical to the old set
        $cmd = "$rwsetcat --cidr-blocks=1 $oldsets{$s}";
        compute_md5(\$md5, $cmd);
        $cmd = "$rwsettool $common_args --intersect $sets{$s} $oldsets{$s}"
            ." | $rwsetcat --cidr-blocks=1";
        check_md5_output($md5, $cmd);
        $cmd = "$rwsettool $common_args --intersect $oldsets{$s} $sets{$s}"
            ." | $rwsetcat --cidr-blocks=1";
        check_md5_output($md5, $cmd);
    }

    unless ($ENV{SK_TESTS_SAVEOUTPUT}) {
        unlink "$tmpdir/$f", values %oldsets;
    }
}
closedir T;


#### CHECK THE HASHES

my %md5s = (
    sip => 'cd161b9ab65ec1a57a417657f2d1d9f5',
    dip => '0b8ce35546b2a64ec773b764fa708e4f',
    any => '95321bb128f196eba35f909124741ea9',
    );
for my $s (keys %sets) {
    $md5 = $md5s{$s};
    $cmd = "$rwsetcat --cidr $sets{$s}";
    check_md5_output($md5, $cmd);
}
