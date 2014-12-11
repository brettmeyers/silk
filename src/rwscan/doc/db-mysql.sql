-- MySQL Database schema for the SiLK scan detection system (rwscan)
-- Last modified: $SiLK: db-mysql.sql 76792f968aea 2007-05-16 17:35:54Z mthomas $

CREATE TABLE scans (
    id          integer unsigned    not null auto_increment,
    sip         integer unsigned    not null,
    proto       tinyint unsigned    not null,
    stime       datetime            not null,
    etime       datetime            not null,
    flows       integer unsigned    not null,
    packets     integer unsigned    not null,
    bytes       integer unsigned    not null,
    scan_model  integer unsigned    not null,
    scan_prob   float unsigned      not null,
    primary key (id),
    INDEX (stime),
    INDEX (etime)
) TYPE=InnoDB;
