#! /usr/bin/perl -w
# MD5: multiple
# TEST: multiple

use strict;
use SiLKTests;

# verify that required features are available
check_features(qw(ipset_v6));

# find the apps we need.  this will exit 77 if they're not available
my $rwset = check_silk_app('rwset');
my $rwsetcat = check_silk_app('rwsetcat');
my $rwsettool = check_silk_app('rwsettool');

# find the data files we use as sources, or exit 77
my %file;
$file{v6data} = get_data_or_exit77('v6data');

my %temp;
$temp{sipset} = make_tempname('sipset');
$temp{sampleset} = make_tempname('sampleset');

# result of running MD5 on /dev/null
my $empty_md5 = 'd41d8cd98f00b204e9800998ecf8427e';

my ($cmd, $md5);

# create source set from the data-v6.rwf
$cmd = "$rwset --sip-file=$temp{sipset} $file{v6data}";
check_md5_output($empty_md5, $cmd);

# create sample set
$cmd = ("$rwsettool --sample --ratio=0.02 --seed=2749473 $temp{sipset}"
        ." --output-path=$temp{sampleset}");
check_md5_output($empty_md5, $cmd);

# Intersection of sample set with source set should be sample set
$cmd = "$rwsetcat $temp{sampleset}";
compute_md5(\$md5, $cmd);

$cmd = ("$rwsettool --intersect $temp{sipset} $temp{sampleset}"
        ." | $rwsetcat");
check_md5_output($md5, $cmd);

# count IPs in the set
$cmd = "$rwsetcat --count $temp{sampleset}";
compute_md5(\$md5, $cmd);

# cannot check the md5 since it will vary across platforms

