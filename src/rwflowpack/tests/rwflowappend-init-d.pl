#! /usr/bin/perl -w
#
#
# RCSIDENT("$SiLK: rwflowappend-init-d.pl f2d4deeba366 2014-11-06 20:27:08Z mthomas $")

use strict;
use SiLKTests;

my $NAME = $0;
$NAME =~ s,.*/,,;

# create our tempdir
my $tmpdir = make_tempdir();

# the daemon being tested and the DAEMON.init.d and DAEMON.conf files
my $DAEMON = 'rwflowappend';

my $daemon_init = "$DAEMON.init.d";
unless (-x $daemon_init) {
    skip_test("Missing start-up script '$daemon_init'");
}
check_daemon_init_program_name($daemon_init, $DAEMON);

my $daemon_src = "$DAEMON.conf";
unless (-f $daemon_src) {
    skip_test("Missing template file '$daemon_src'");
}

my $daemon_conf = "$tmpdir/$DAEMON.conf";

# set environment variable to the directory holding $DAEMON.conf
$ENV{SCRIPT_CONFIG_LOCATION} = $tmpdir;


# directories
my $log_dir = "$tmpdir/log";
my $data_dir = "$tmpdir/data";

# create the data directory and copy the silk.conf file
if (-f $ENV{SILK_CONFIG_FILE}) {
    mkdir $data_dir, 0700
        or die "$NAME: Unable to create directory '$data_dir': $!\n";
    system "cp", $ENV{SILK_CONFIG_FILE}, "$data_dir/silk.conf";
}

# open the template file for $DAEMON.conf
open SRC, $daemon_src
    or die "$NAME: Cannot open template file '$daemon_src': $!\n";

# create $DAEMON.conf
my $daemon_text;
while (<SRC>) {
    chomp;
    s/\#.*//;
    next unless /\S/;

    if (/^(BIN_DIR=).*/) {
        my $pwd = `pwd`;
        $daemon_text .= $1 . $pwd;
        next;
    }
    if (/^(CREATE_DIRECTORIES=).*/) {
        $daemon_text .= $1 . "yes\n";
        next;
    }
    if (/^(ENABLED=).*/) {
        $daemon_text .= $1 . "yes\n";
        next;
    }
    if (/^(LOG_TYPE=).*/) {
        $daemon_text .= $1 . "legacy\n";
        next;
    }
    if (/^(statedirectory=).*/) {
        $daemon_text .= $1 . $tmpdir . "\n";
        next;
    }
    if (/^(USER=).*/) {
        $daemon_text .= $1 . '`whoami`' . "\n";
        next;
    }

    if (/^(DATA_ROOTDIR=).*/) {
        $daemon_text .= $1 . $data_dir . "\n";
        next;
    }
    if (/^(ARCHIVE_DIR=).*/) {
        $daemon_text .= $1 . '${statedirectory}/archive' . "\n";
        next;
    }
    if (/^((POST_COMMAND|HOUR_COMMAND)=).*/) {
        $daemon_text .= $1 . "'echo \%s'\n";
        next;
    }
    if (/^(REJECT_HOURS_(PAST|FUTURE)=).*/) {
        $daemon_text .= $1 . "5\n";
        next;
    }
    if (/^(APPEND_THREADS=).*/) {
        $daemon_text .= $1 . "4\n";
        next;
    }

    $daemon_text .= $_ ."\n";
}
close SRC;
make_config_file($daemon_conf, \$daemon_text);


my $cmd;
my $expected_status;
my $good = 1;
my $pid;

# run "daemon.init.d status"; it should not be running
$expected_status = 'stopped';
$cmd = "./$daemon_init status";
run_command($cmd, \&init_d);
exit 1
    unless $good;

# run "daemon.init.d start"
$expected_status = 'starting';
$cmd = "./$daemon_init start";
run_command($cmd, \&init_d);

# get the PID from the pidfile
my $pidfile = "$log_dir/$DAEMON.pid";
if (-f $pidfile) {
    open PID, $pidfile
        or die "$NAME: Cannot open '$pidfile': $!\n";
    $pid = "";
    while (my $x = <PID>) {
        $pid .= $x;
    }
    close PID;
    if ($pid && $pid =~ /^(\d+)/) {
        $pid = $1;
    }
    else {
        $pid = undef;
    }
}

# kill the process if the daemon.init.d script failed
unless ($good) {
    if ($pid) {
        kill_process($pid);
    }
    exit 1;
}

sleep 2;

# run "daemon.init.d stutus"; it should be running
$expected_status = 'running';
$cmd = "./$daemon_init status";
run_command($cmd, \&init_d);

sleep 2;

$expected_status = 'stopping';
$cmd = "./$daemon_init stop";
run_command($cmd, \&init_d);

sleep 2;

# run "daemon.init.d status"; it should not be running
$expected_status = 'stopped';
$cmd = "./$daemon_init status";
run_command($cmd, \&init_d);

if ($pid) {
    if (kill 0, $pid) {
        print STDERR "$NAME: Process $pid is still running\n";
        $good = 0;
        kill_process($pid);
    }
}

check_log_stopped();

exit !$good;


sub init_d
{
    my ($io) = @_;

    while (my $line = <$io>) {
        if ($ENV{SK_TESTS_VERBOSE}) {
            print STDERR $line;
        }
        chomp $line;

        if ($expected_status eq 'running') {
            if ($line =~ /$DAEMON is running with pid (\d+)/) {
                if ($pid) {
                    if ($pid ne $1) {
                        print STDERR ("$NAME: Process ID mismatch:",
                                      " file '$pid' vs script '$1'\n");
                        $good = 0;
                    }
                }
                else {
                    $pid = $1;
                }
            }
            else {
                print STDERR
                    "$NAME: Unexpected $expected_status line '$line'\n";
                $good = 0;
            }
        }
        elsif ($expected_status eq 'starting') {
            unless ($line =~ /Starting $DAEMON:\t\[OK\]/) {
                print STDERR
                    "$NAME: Unexpected $expected_status line '$line'\n";
                $good = 0;
            }
        }
        elsif ($expected_status eq 'stopped') {
            unless ($line =~ /$DAEMON is stopped/) {
                print STDERR
                    "$NAME: Unexpected $expected_status line '$line'\n";
                $good = 0;
            }
        }
        elsif ($expected_status eq 'stopping') {
            unless ($line =~ /Stopping $DAEMON:\t\[OK\]/) {
                print STDERR
                    "$NAME: Unexpected $expected_status line '$line'\n";
                $good = 0;
            }
        }
        else {
            die "$NAME: Unknown \$expected_status '$expected_status'\n";
        }
    }

    close $io;
    if ($expected_status eq 'running') {
        unless ($? == 0) {
            print STDERR "$NAME: Unexpected $expected_status status $?\n";
            $good = 0;
        }
    }
    elsif ($expected_status eq 'starting') {
        unless ($? == 0) {
            print STDERR "$NAME: Unexpected $expected_status status $?\n";
            $good = 0;
        }
    }
    elsif ($expected_status eq 'stopped') {
        unless ($? == (1 << 8)) {
            print STDERR "$NAME: Unexpected $expected_status status $?\n";
            $good = 0;
        }
    }
    elsif ($expected_status eq 'stopping') {
        unless ($? == (1 << 8)) {
            print STDERR "$NAME: Unexpected $expected_status status $?\n";
            $good = 0;
        }
    }

    if ($ENV{SK_TESTS_VERBOSE}) {
        print STDERR "status = $?\n";
    }
}


sub kill_process
{
    my ($pid) = @_;

    if (!kill 0, $pid) {
        return;
    }
    if ($ENV{SK_TESTS_VERBOSE}) {
        print STDERR "Sending SIGTERM to process $pid\n";
    }
    kill 15, $pid;
    sleep 2;
    if (kill 0, $pid) {
        if ($ENV{SK_TESTS_VERBOSE}) {
            print STDERR "Sending SIGKILL to process $pid\n";
        }
        kill 9, $pid;
    }
}


sub check_log_stopped
{
    my @log_files = sort glob("$log_dir/$DAEMON-*.log");
    my $log = pop @log_files;
    open LOG, $log
        or die "$NAME: Cannot open log file '$log': $!\n";
    my $last_line;
    while (defined(my $line = <LOG>)) {
        $last_line = $line;
    }
    close LOG;
    unless (defined $last_line) {
        die "$NAME: Log file '$log' is empty\n";
    }
    if ($last_line !~ /Stopped logging/) {
        die "$NAME: Log file '$log' does not end correctly\n";
    }
}

