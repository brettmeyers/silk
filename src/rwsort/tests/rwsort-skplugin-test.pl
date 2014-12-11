#! /usr/bin/perl -w
# MD5: 6d5de2000d6473006117de73bb698329
# TEST: ./rwsort --plugin=skplugin-test.so --fields=copy-bytes ../../tests/data.rwf | ../rwuniq/rwuniq --plugin=skplugin-test.so --ipv6-policy=ignore --fields=copy-bytes --values=bytes,packets,records --presorted-input

use strict;
use SiLKTests;

my $rwsort = check_silk_app('rwsort');
my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load skplugin-test.so plugin')
    unless check_app_switch($rwsort.' --plugin=skplugin-test.so', 'fields', qr/copy-bytes/);
my $cmd = "$rwsort --plugin=skplugin-test.so --fields=copy-bytes $file{data} | $rwuniq --plugin=skplugin-test.so --ipv6-policy=ignore --fields=copy-bytes --values=bytes,packets,records --presorted-input";
my $md5 = "6d5de2000d6473006117de73bb698329";

check_md5_output($md5, $cmd);
