#! /usr/bin/perl -w
# MD5: 74e25da3a90db40104b70821c4fb8ab3
# TEST: ../rwfilter/rwfilter --daddr=192.168.x.x --sport=0-1024 --pass=stdout ../../tests/data.rwf | ../rwsort/rwsort --fields=1,4,2,3,5,9 --output-path=/tmp/rwmatch-ext-server-incoming && ../rwfilter/rwfilter --saddr=192.168.x.x --dport=0-1024 --pass=stdout ../../tests/data.rwf | ../rwsort/rwsort --fields=2,3,1,4,5,9 --output-path=/tmp/rwmatch-ext-server-outgoing && ./rwmatch --time-delta=2.5 --symmetric-del --relative-del --relate=2,1 --relate=3,4 --relate=1,2 --relate=4,3 --relate=5,5 /tmp/rwmatch-ext-server-outgoing /tmp/rwmatch-ext-server-incoming - | ../rwcat/rwcat --compression-method=none --byte-order=little --ipv4-output

use strict;
use SiLKTests;

my $rwmatch = check_silk_app('rwmatch');
my $rwcat = check_silk_app('rwcat');
my $rwfilter = check_silk_app('rwfilter');
my $rwsort = check_silk_app('rwsort');
my %file;
$file{data} = get_data_or_exit77('data');
my %temp;
$temp{incoming} = make_tempname('incoming');
$temp{outgoing} = make_tempname('outgoing');
my $cmd = "$rwfilter --daddr=192.168.x.x --sport=0-1024 --pass=stdout $file{data} | $rwsort --fields=1,4,2,3,5,9 --output-path=$temp{incoming} && $rwfilter --saddr=192.168.x.x --dport=0-1024 --pass=stdout $file{data} | $rwsort --fields=2,3,1,4,5,9 --output-path=$temp{outgoing} && $rwmatch --time-delta=2.5 --symmetric-del --relative-del --relate=2,1 --relate=3,4 --relate=1,2 --relate=4,3 --relate=5,5 $temp{outgoing} $temp{incoming} - | $rwcat --compression-method=none --byte-order=little --ipv4-output";
my $md5 = "74e25da3a90db40104b70821c4fb8ab3";

check_md5_output($md5, $cmd);
