-- PostgreSQL Database schema for the SiLK scan detection system (rwscan)
-- Last modified: $SiLK: db-postgres.sql 848d973cdf0c 2008-01-29 22:48:03Z tonyc $

CREATE SCHEMA scans

CREATE SEQUENCE scans_id_seq

CREATE TABLE scans (
    id          BIGINT      NOT NULL    DEFAULT nextval('scans_id_seq'),
    sip         BIGINT      NOT NULL,
    proto       SMALLINT    NOT NULL,
    stime       TIMESTAMP without time zone NOT NULL,
    etime       TIMESTAMP without time zone NOT NULL,
    flows       BIGINT      NOT NULL,
    packets     BIGINT      NOT NULL,
    bytes       BIGINT      NOT NULL,
    scan_model  INTEGER     NOT NULL,
    scan_prob   FLOAT       NOT NULL,
    PRIMARY KEY (id)
)

CREATE INDEX scans_stime_idx ON scans (stime)
CREATE INDEX scans_etime_idx ON scans (etime)
;
