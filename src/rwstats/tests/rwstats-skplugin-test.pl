#! /usr/bin/perl -w
# MD5: dd0a3031cfef23773fbb45dd9cc6f05e
# TEST: ./rwstats --plugin=skplugin-test.so --fields=copy-bytes --values=bytes,packets,records --count=10 ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwstats = check_silk_app('rwstats');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load skplugin-test.so plugin')
    unless check_app_switch($rwstats.' --plugin=skplugin-test.so', 'fields', qr/copy-bytes/);
my $cmd = "$rwstats --plugin=skplugin-test.so --fields=copy-bytes --values=bytes,packets,records --count=10 $file{data}";
my $md5 = "dd0a3031cfef23773fbb45dd9cc6f05e";

check_md5_output($md5, $cmd);
