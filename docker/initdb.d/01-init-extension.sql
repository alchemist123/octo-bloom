-- Initialize octo-bloom extension
-- This script runs automatically when the PostgreSQL container starts

-- Create the extension
CREATE EXTENSION IF NOT EXISTS octo_bloom;

-- Create a sample table for testing
CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    email VARCHAR(255) UNIQUE NOT NULL,
    username VARCHAR(100) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Insert some sample data
INSERT INTO users (email, username) VALUES 
    ('alice@example.com', 'alice'),
    ('bob@example.com', 'bob'),
    ('charlie@example.com', 'charlie'),
    ('diana@example.com', 'diana'),
    ('eve@example.com', 'eve'),
    ('frank@example.com', 'frank'),
    ('grace@example.com', 'grace'),
    ('henry@example.com', 'henry'),
    ('iris@example.com', 'iris'),
    ('jack@example.com', 'jack')
ON CONFLICT DO NOTHING;

-- Initialize bloom filter for the users table email column
SELECT octo_bloom_init('users'::regclass, 'email', 2, 0.01);

-- Create another sample table for more testing
CREATE TABLE IF NOT EXISTS products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    sku VARCHAR(100) UNIQUE NOT NULL,
    price DECIMAL(10,2),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Insert sample products
INSERT INTO products (name, sku, price) VALUES 
    ('Laptop', 'LAP001', 999.99),
    ('Mouse', 'MOU001', 29.99),
    ('Keyboard', 'KEY001', 79.99),
    ('Monitor', 'MON001', 299.99),
    ('Headphones', 'HEAD001', 149.99),
    ('Webcam', 'WEB001', 89.99),
    ('Tablet', 'TAB001', 399.99),
    ('Phone', 'PHO001', 699.99),
    ('Speaker', 'SPK001', 199.99),
    ('Router', 'ROU001', 129.99)
ON CONFLICT DO NOTHING;

-- Initialize bloom filter for the products table sku column
SELECT octo_bloom_init('products'::regclass, 'sku', 2, 0.01);

-- Grant permissions
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO octo_user;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO octo_user;
