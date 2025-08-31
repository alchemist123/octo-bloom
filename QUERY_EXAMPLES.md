# Octo-Bloom PostgreSQL Extension - Query Examples

This document provides a complete walkthrough of using the Octo-Bloom PostgreSQL extension with example queries in the correct order.

## Prerequisites

Make sure the extension is built and installed:

```bash
# Build and install the extension
make clean && make install

# Connect to PostgreSQL
psql -d postgres
```

## Step 1: Create Extension

```sql
-- Create the Octo-Bloom extension
CREATE EXTENSION octo_bloom;
```

## Step 2: Create Test Table

```sql
-- Create a sample table for testing
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    email VARCHAR(255) UNIQUE NOT NULL,
    name VARCHAR(100)
);
```

## Step 3: Initialize Bloom Filter

```sql
-- Initialize bloom filter for the 'email' column of 'users' table
-- Parameters: table_name, column_name, expected_count, false_positive_rate
SELECT octo_bloom_init('users', 'email', 1000, 0.01);
```

**Parameters Explanation:**
- `'users'`: Table name (regclass type)
- `'email'`: Column name to create bloom filter for
- `1000`: Expected number of elements
- `0.01`: False positive rate (1%)

## Step 4: Test Empty Bloom Filter

```sql
-- Test might_contain on empty bloom filter (should return true for empty filter)
SELECT octo_bloom_might_contain('users', 'email', 'test@example.com'::text);
-- Result: t (true)

-- Test exists on empty table (should return false)
SELECT octo_bloom_exists('users', 'email', 'test@example.com'::text);
-- Result: f (false)
```

## Step 5: Insert Test Data

```sql
-- Insert sample data
INSERT INTO users (email, name) VALUES 
    ('alice@example.com', 'Alice'),
    ('bob@example.com', 'Bob'),
    ('charlie@example.com', 'Charlie'),
    ('diana@example.com', 'Diana'),
    ('eve@example.com', 'Eve');
```

## Step 6: Test Existing Data

```sql
-- Test with existing email addresses
SELECT 'alice@example.com' as email,
       octo_bloom_might_contain('users', 'email', 'alice@example.com'::text) as might_contain,
       octo_bloom_exists('users', 'email', 'alice@example.com'::text) as exists;
-- Expected: might_contain=t, exists=t

SELECT 'bob@example.com' as email,
       octo_bloom_might_contain('users', 'email', 'bob@example.com'::text) as might_contain,
       octo_bloom_exists('users', 'email', 'bob@example.com'::text) as exists;
-- Expected: might_contain=t, exists=t
```

## Step 7: Test Non-Existent Data

```sql
-- Test with non-existent email addresses
SELECT 'nonexistent@test.com' as email,
       octo_bloom_might_contain('users', 'email', 'nonexistent@test.com'::text) as might_contain,
       octo_bloom_exists('users', 'email', 'nonexistent@test.com'::text) as exists;
-- Expected: might_contain=t or f (probabilistic), exists=f (definitive)

SELECT 'fake@example.org' as email,
       octo_bloom_might_contain('users', 'email', 'fake@example.org'::text) as might_contain,
       octo_bloom_exists('users', 'email', 'fake@example.org'::text) as exists;
-- Expected: might_contain=t or f (probabilistic), exists=f (definitive)
```

## Step 8: Comprehensive Test Query

```sql
-- Comprehensive test showing all functionality
SELECT '=== OCTO-BLOOM EXTENSION TEST RESULTS ===' as summary;

-- Show current data
SELECT 'Current users in table:' as info;
SELECT id, email, name FROM users ORDER BY id;

-- Test existing emails
SELECT 'TEST 1: Existing emails' as test_type;
SELECT email,
       octo_bloom_might_contain('users', 'email', email::text) as bloom_result,
       octo_bloom_exists('users', 'email', email::text) as definitive_result
FROM users 
LIMIT 3;

-- Test non-existent emails
SELECT 'TEST 2: Non-existent emails' as test_type;

SELECT 'fake1@test.com' as email,
       octo_bloom_might_contain('users', 'email', 'fake1@test.com'::text) as bloom_result,
       octo_bloom_exists('users', 'email', 'fake1@test.com'::text) as definitive_result;

SELECT 'fake2@test.com' as email,
       octo_bloom_might_contain('users', 'email', 'fake2@test.com'::text) as bloom_result,
       octo_bloom_exists('users', 'email', 'fake2@test.com'::text) as definitive_result;

SELECT 'Extension working correctly!' as result;
```

## Step 9: Performance Comparison (Optional)

```sql
-- Compare performance: Direct query vs Bloom filter + query
-- For large datasets, this shows the performance benefit

-- Direct query (always scans)
EXPLAIN ANALYZE SELECT EXISTS(SELECT 1 FROM users WHERE email = 'nonexistent@test.com');

-- Bloom filter + query (may skip scan if bloom filter says "no")
EXPLAIN ANALYZE SELECT octo_bloom_exists('users', 'email', 'nonexistent@test.com'::text);
```

## Step 10: Reinitialize Bloom Filter (Optional)

```sql
-- Reinitialize bloom filter with different parameters
SELECT octo_bloom_init('users', 'email', 5000, 0.001);
-- This updates the existing filter with new parameters
```

## Function Reference

### octo_bloom_init(table_name, column_name, expected_count, false_positive_rate)
- **Purpose**: Initialize a bloom filter for a specific table column
- **Returns**: void
- **Parameters**:
  - `table_name` (regclass): Name of the table
  - `column_name` (text): Name of the column
  - `expected_count` (bigint): Expected number of elements
  - `false_positive_rate` (float): Desired false positive rate (0-1)

### octo_bloom_might_contain(table_name, column_name, value)
- **Purpose**: Fast probabilistic membership test
- **Returns**: boolean (true if might exist, false if definitely doesn't exist)
- **Note**: No false negatives, but may have false positives

### octo_bloom_exists(table_name, column_name, value)
- **Purpose**: Definitive existence check using bloom filter + database query
- **Returns**: boolean (true if exists, false if doesn't exist)
- **Note**: Combines bloom filter optimization with database verification

## Expected Results Summary

| Test Case | might_contain | exists | Explanation |
|-----------|---------------|--------|-------------|
| Existing data | `true` | `true` | Data exists in both filter and database |
| Non-existent data | `true/false` | `false` | Filter may give false positive, but DB query is definitive |
| Empty filter | `true` | `false` | Empty filter returns true for might_contain |

## Troubleshooting

### Common Issues

1. **"could not determine polymorphic type"**
   ```sql
   -- Wrong: SELECT octo_bloom_might_contain('users', 'email', 'test@example.com');
   -- Correct: Cast the value
   SELECT octo_bloom_might_contain('users', 'email', 'test@example.com'::text);
   ```

2. **"relation does not exist"**
   ```sql
   -- Make sure table exists before initializing bloom filter
   CREATE TABLE users (...);
   ```

3. **"column does not exist"**
   ```sql
   -- Make sure column name is correct
   SELECT octo_bloom_init('users', 'email', 1000, 0.01); -- correct column name
   ```

## Performance Notes

- **octo_bloom_might_contain()**: O(k) where k is number of hash functions (~constant time)
- **octo_bloom_exists()**: O(k) + O(database query time) - but may skip database query entirely
- **Memory usage**: Compact bit array representation, configurable based on expected count and false positive rate
- **Best use case**: Large datasets where you frequently check for non-existent values

## Cleanup

```sql
-- Drop extension (removes all bloom filters)
DROP EXTENSION octo_bloom CASCADE;

-- Drop test table
DROP TABLE users;
```
