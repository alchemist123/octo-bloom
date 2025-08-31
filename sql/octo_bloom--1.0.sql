-- Octo-Bloom PostgreSQL Extension
-- SQL functions and setup

CREATE OR REPLACE FUNCTION octo_bloom_init(
    table_oid regclass,
    column_name text,
    expected_count bigint DEFAULT 1000000,
    false_positive_rate float DEFAULT 0.01
) RETURNS void
AS 'octo_bloom', 'octo_bloom_init'
LANGUAGE C STRICT;

-- Polymorphic functions (original)
CREATE OR REPLACE FUNCTION octo_bloom_might_contain(
    table_oid regclass,
    column_name text,
    value anyelement
) RETURNS boolean
AS 'octo_bloom', 'octo_bloom_might_contain'
LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION octo_bloom_exists(
    table_oid regclass,
    column_name text,
    value anyelement
) RETURNS boolean
AS 'octo_bloom', 'octo_bloom_exists'
LANGUAGE C;

-- Type-specific overloads for common types
CREATE OR REPLACE FUNCTION octo_bloom_might_contain(
    table_oid regclass,
    column_name text,
    value text
) RETURNS boolean
AS 'octo_bloom', 'octo_bloom_might_contain'
LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION octo_bloom_might_contain(
    table_oid regclass,
    column_name text,
    value integer
) RETURNS boolean
AS 'octo_bloom', 'octo_bloom_might_contain'
LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION octo_bloom_might_contain(
    table_oid regclass,
    column_name text,
    value bigint
) RETURNS boolean
AS 'octo_bloom', 'octo_bloom_might_contain'
LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION octo_bloom_might_contain(
    table_oid regclass,
    column_name text,
    value varchar
) RETURNS boolean
AS 'octo_bloom', 'octo_bloom_might_contain'
LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION octo_bloom_exists(
    table_oid regclass,
    column_name text,
    value text
) RETURNS boolean
AS 'octo_bloom', 'octo_bloom_exists'
LANGUAGE C;

CREATE OR REPLACE FUNCTION octo_bloom_exists(
    table_oid regclass,
    column_name text,
    value integer
) RETURNS boolean
AS 'octo_bloom', 'octo_bloom_exists'
LANGUAGE C;

CREATE OR REPLACE FUNCTION octo_bloom_exists(
    table_oid regclass,
    column_name text,
    value bigint
) RETURNS boolean
AS 'octo_bloom', 'octo_bloom_exists'
LANGUAGE C;

CREATE OR REPLACE FUNCTION octo_bloom_exists(
    table_oid regclass,
    column_name text,
    value varchar
) RETURNS boolean
AS 'octo_bloom', 'octo_bloom_exists'
LANGUAGE C;