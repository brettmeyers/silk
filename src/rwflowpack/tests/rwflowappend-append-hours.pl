#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: rwflowappend-append-hours.pl 40a363507ed0 2014-04-01 14:09:52Z mthomas $")

use strict;
use SiLKTests;
use File::Temp ();


# find the apps we need.  this will exit 77 if they're not available
my $rwtuc = check_silk_app('rwtuc');

# prefix any existing PYTHONPATH with the proper directories
check_python_bin();

# create our tempdir
my $tmpdir = make_tempdir();

# the incremental files
my %input_files;

# the relative directory we expect
my %dirpath;

# get current time
my $t1 = time;

# create 16 files whose times differ from each other by two hours and
# range from 20 hours ago to 10 hours into future.  Stepping by two
# hours avoids issues if hour rolls over during the test
for (my $h = -20; $h <= 10; $h += 2) {
    my $ht = $t1 + $h * 3600;
    my @gmt = gmtime($ht);
    $dirpath{$h} =  sprintf("in/%04d/%02d/%02d",
                            $gmt[5] + 1900, $gmt[4] + 1, $gmt[3]);
    my $f = sprintf("%s/in-S8_%04d%02d%02d.%02d.XXXXXX",
                    $tmpdir, $gmt[5] + 1900, $gmt[4] + 1, $gmt[3], $gmt[2]);
    $input_files{$h} = File::Temp::mktemp($f);
    my $cmd = ("echo 10.$gmt[4].$gmt[3].$gmt[2],$ht"
               ." | $rwtuc --fields=sip,stime --column-sep=,"
               ." --output-path=$input_files{$h}");
    check_md5_output('d41d8cd98f00b204e9800998ecf8427e', $cmd);
}

# time window outside of which to reject data.
#
# the following causes the test to pass 11 files and fail 5 files,
# which is required for the MD5 to succeed.
my $reject_future = 2 * int(rand 6);
if (defined $ENV{REJECT_FUTURE} && $ENV{REJECT_FUTURE} =~ /^(\d+)/) {
    if ($1 > 5) {
        die "Maximum value for REJECT_FUTURE is 5\n";
    }
    $reject_future = $1;
}

# use 21 instead of 20 in case the hour rolls over
my $reject_past = 21 - $reject_future;

# don't provide the switch when we're at the edge of the time window
if ($reject_future == 10) {
    undef $reject_future;
}
elsif ($reject_past == 21) {
    undef $reject_past;
}

# the command that wraps rwflowpack
my $cmd = join " ", ("$SiLKTests::PYTHON $srcdir/tests/rwflowappend-daemon.py",
                     ($ENV{SK_TESTS_VERBOSE} ? "--verbose" : ()),
                     ($ENV{SK_TESTS_LOG_DEBUG} ? "--log-level=debug" : ()),
                     "--daemon-timeout=15",
                     "--basedir=$tmpdir",
                     (map {"--copy $input_files{$_}:incoming"}
                      keys %input_files),
                     "--",
                     ((defined $reject_past)
                      ? "--reject-hours-past=$reject_past"
                      : ()),
                     ((defined $reject_future)
                      ? "--reject-hours-future=$reject_future"
                      : ()),
                     "--polling-interval=5",
    );

# run it
check_md5_output('cc4fbfb7b7fc7d2d063da4488a5df1b8', $cmd);

# the following directories should be empty
verify_empty_dirs($tmpdir, qw(incoming));

# verify files are in proper directory
my $archive_dir = "$tmpdir/archive";
my $data_dir = "$tmpdir/root";
my $error_dir = "$tmpdir/error";

for my $k (sort {$a <=> $b} keys %input_files) {
    if ((defined($reject_past) && $k < -$reject_past)
        || (defined($reject_future) && $k > $reject_future))
    {
        # should be in error directory
        my $f = $input_files{$k};
        $f =~ s,.*/,$error_dir/,;
        die "ERROR: Missing error file '$f'\n"
            unless -f $f;
    } else {
        # should be in archive and data directories
        my $f = $input_files{$k};
        $f =~ s,.*/,$archive_dir/$dirpath{$k}/,;
        die "ERROR: Missing archive file '$f'\n"
            unless -f $f;
        $f = $input_files{$k};
        $f =~ s,.*/,$data_dir/$dirpath{$k}/,;
        $f =~ s/(\.\d\d)\.......$/$1/;
        die "ERROR: Missing data file '$f'\n"
            unless -f $f;
    }
}
