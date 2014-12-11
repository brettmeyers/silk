#! /usr/bin/perl -w
# MD5: 51236d2af079ddc7f3e29172d14f7bb8
# TEST: ./rwuniq --plugin=skplugin-test.so --ipv6-policy=ignore --no-column --fields=protocol --values=bytes,sum-bytes,min-bytes,max-bytes,weird-bytes --sort-output ../../tests/data.rwf

use strict;
use SiLKTests;

my $rwuniq = check_silk_app('rwuniq');
my %file;
$file{data} = get_data_or_exit77('data');
add_plugin_dirs('/src/plugins');

skip_test('Cannot load skplugin-test.so plugin')
    unless check_app_switch($rwuniq.' --plugin=skplugin-test.so', 'fields', qr/copy-bytes/);
my $cmd = "$rwuniq --plugin=skplugin-test.so --ipv6-policy=ignore --no-column --fields=protocol --values=bytes,sum-bytes,min-bytes,max-bytes,weird-bytes --sort-output $file{data}";
my $md5 = "51236d2af079ddc7f3e29172d14f7bb8";

check_md5_output($md5, $cmd);
