# Octo-Bloom Docker Setup

This directory contains the Docker setup for running Octo-Bloom PostgreSQL extension with pgAdmin for easy testing and development.

## 🐳 Docker Version Requirements

- **Docker**: 20.10.0 or later
- **Docker Compose**: 2.0.0 or later

Check your versions:
```bash
docker --version
docker-compose --version
```

## 🚀 Quick Start

### 1. Clone and Navigate
```bash
git clone <your-repo-url>
cd octo-bloom
```

### 2. Start Services
```bash
# Start PostgreSQL + pgAdmin
docker-compose up -d

# Check services are running
docker-compose ps
```

### 3. Access Services
- **PostgreSQL**: `localhost:5432`
- **pgAdmin**: http://localhost:8080

## 📋 Available Commands

### Service Management
```bash
# Start all services
docker-compose up -d

# Stop all services
docker-compose down

# Stop and remove volumes (clears all data)
docker-compose down -v

# View logs
docker-compose logs -f

# View logs for specific service
docker-compose logs -f postgres
docker-compose logs -f pgadmin

# Restart services
docker-compose restart

# Rebuild containers (after code changes)
docker-compose build --no-cache
docker-compose up -d
```

### Database Access
```bash
# Connect to PostgreSQL via command line
docker exec -it octo-bloom-postgres psql -U octo_user -d octo_bloom_db

# Run single SQL command
docker exec -it octo-bloom-postgres psql -U octo_user -d octo_bloom_db -c "SELECT version();"

# Execute SQL file
docker exec -i octo-bloom-postgres psql -U octo_user -d octo_bloom_db < your_script.sql
```

## 🔧 Configuration

### Environment Variables
Copy and customize the environment file:
```bash
cp docker.env.example .env
```

Default configuration:
```env
# PostgreSQL Configuration
POSTGRES_DB=octo_bloom_db
POSTGRES_USER=octo_user
POSTGRES_PASSWORD=octo_pass123
POSTGRES_PORT=5432

# pgAdmin Configuration
PGADMIN_EMAIL=admin@octo-bloom.com
PGADMIN_PASSWORD=admin123
PGADMIN_PORT=8080
```

### Service Details

#### PostgreSQL Container
- **Image**: Custom built with Octo-Bloom extension
- **Port**: 5432
- **Database**: octo_bloom_db
- **User**: octo_user
- **Password**: octo_pass123

#### pgAdmin Container
- **Image**: dpage/pgadmin4:latest
- **Port**: 8080
- **Email**: admin@octo-bloom.com
- **Password**: admin123

## 🧪 Demo Queries & Testing

### 1. Basic Extension Check
```bash
docker exec -it octo-bloom-postgres psql -U octo_user -d octo_bloom_db -c "\dx"
```

### 2. View Sample Data
```sql
-- Connect first
docker exec -it octo-bloom-postgres psql -U octo_user -d octo_bloom_db

-- Then run these queries:
SELECT * FROM users LIMIT 5;
SELECT * FROM products LIMIT 5;

-- Check table structures
\d users
\d products
```

### 3. Bloom Filter Function Tests

#### Test Email Bloom Filter
```sql
-- Test with existing email (should return true)
SELECT octo_bloom_might_contain('users'::regclass, 'email', 'alice@example.com'::text);

-- Test with non-existing email (might return true due to false positives)
SELECT octo_bloom_might_contain('users'::regclass, 'email', 'nonexistent@example.com'::text);

-- Test with obviously non-existing email (likely false)
SELECT octo_bloom_might_contain('users'::regclass, 'email', 'totally-fake-12345@nowhere.invalid'::text);
```

#### Test Product SKU Bloom Filter
```sql
-- Test with existing SKU
SELECT octo_bloom_might_contain('products'::regclass, 'sku', 'LAP001'::text);

-- Test with non-existing SKU
SELECT octo_bloom_might_contain('products'::regclass, 'sku', 'FAKE123'::text);
```

#### Test Different Data Types
```sql
-- Text values
SELECT octo_bloom_might_contain('users'::regclass, 'username', 'alice'::text);

-- Integer values (if you have integer columns)
SELECT octo_bloom_might_contain('users'::regclass, 'id', 1::integer);

-- Using octo_bloom_exists function
SELECT octo_bloom_exists('users'::regclass, 'email', 'bob@example.com'::text);
```

### 4. Performance Testing
```sql
-- Time the bloom filter check
\timing on
SELECT octo_bloom_might_contain('users'::regclass, 'email', 'alice@example.com'::text);

-- Compare with actual database lookup
SELECT EXISTS(SELECT 1 FROM users WHERE email = 'alice@example.com');
\timing off
```

### 5. Bulk Testing Script
```sql
-- Test multiple values at once
WITH test_emails AS (
  SELECT unnest(ARRAY[
    'alice@example.com',
    'bob@example.com', 
    'charlie@example.com',
    'nonexistent1@test.com',
    'nonexistent2@test.com',
    'fake@nowhere.invalid'
  ]) AS email
)
SELECT 
  email,
  octo_bloom_might_contain('users'::regclass, 'email', email::text) AS bloom_result,
  EXISTS(SELECT 1 FROM users u WHERE u.email = test_emails.email) AS actual_exists
FROM test_emails;
```

## 🌐 pgAdmin Web Interface

### Access pgAdmin
1. Open http://localhost:8080
2. Login with:
   - **Email**: admin@octo-bloom.com  
   - **Password**: admin123

### Connect to PostgreSQL
The server should be pre-configured, but if needed:
- **Host**: postgres (container name)
- **Port**: 5432
- **Database**: octo_bloom_db
- **Username**: octo_user
- **Password**: octo_pass123

### Using pgAdmin
1. Navigate to Servers → Octo-Bloom PostgreSQL
2. Browse to Databases → octo_bloom_db → Schemas → public → Tables
3. Use the Query Tool to run SQL commands
4. View data using the built-in data viewer

## 🔍 Troubleshooting

### Common Issues

#### Services Won't Start
```bash
# Check if ports are in use
lsof -i :5432
lsof -i :8080

# Check Docker logs
docker-compose logs postgres
docker-compose logs pgadmin
```

#### Extension Not Loading
```bash
# Rebuild containers
docker-compose down
docker-compose build --no-cache
docker-compose up -d

# Check extension installation
docker exec -it octo-bloom-postgres psql -U octo_user -d octo_bloom_db -c "\dx"
```

#### Permission Issues
```bash
# Reset volumes
docker-compose down -v
docker-compose up -d
```

#### Polymorphic Type Errors
Always cast your values when calling functions:
```sql
-- ❌ Wrong (causes polymorphic type error)
SELECT octo_bloom_might_contain('users'::regclass, 'email', 'test@example.com');

-- ✅ Correct (with type cast)
SELECT octo_bloom_might_contain('users'::regclass, 'email', 'test@example.com'::text);
```

### Health Checks
```bash
# Check container health
docker-compose ps

# Test PostgreSQL connection
docker exec -it octo-bloom-postgres pg_isready -U octo_user -d octo_bloom_db

# Test extension functions
docker exec -it octo-bloom-postgres psql -U octo_user -d octo_bloom_db -c "SELECT octo_bloom_might_contain('users'::regclass, 'email', 'alice@example.com'::text);"
```

## 📊 Understanding Bloom Filters

### What is a Bloom Filter?
A Bloom filter is a space-efficient probabilistic data structure that tells you if an element:
- **Definitely is NOT in the set** (100% accurate)
- **Might be in the set** (with some false positive rate)

### False Positives
The bloom filter may return `true` for items that aren't actually in the dataset. This is normal and expected behavior. The false positive rate is configurable (default: 1%).

### Use Cases
- **Fast membership testing**: Check if an email exists before expensive database lookup
- **Caching**: Avoid cache misses for non-existent data
- **Distributed systems**: Reduce network calls for non-existent data

## 🏗️ Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   pgAdmin       │    │   PostgreSQL     │    │   Octo-Bloom    │
│   (Port 8080)   │◄──►│   (Port 5432)    │◄──►│   Extension     │
│                 │    │                  │    │                 │
│ • Web Interface │    │ • Database       │    │ • Bloom Filters │
│ • Query Tool    │    │ • Sample Data    │    │ • C++ Core      │
│ • Data Viewer   │    │ • Extension      │    │ • Shared Memory │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

## 📝 Sample Data

The setup automatically creates:

### Users Table
```sql
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    email VARCHAR(255) UNIQUE NOT NULL,
    username VARCHAR(100) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### Products Table  
```sql
CREATE TABLE products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    sku VARCHAR(100) UNIQUE NOT NULL,
    price DECIMAL(10,2),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

Both tables come pre-populated with sample data and initialized bloom filters.

## 🔄 Development Workflow

### Making Changes
1. Edit source code in `src/` or SQL in `sql/`
2. Rebuild containers:
   ```bash
   docker-compose down
   docker-compose build --no-cache
   docker-compose up -d
   ```
3. Test your changes using the demo queries above

### Adding New Functions
1. Update `sql/octo_bloom--1.0.sql`
2. Rebuild and restart containers
3. Test new functions in pgAdmin or command line

---

## 🎯 Next Steps

1. **Explore the Extension**: Use the demo queries above
2. **Test Performance**: Compare bloom filter vs direct database queries  
3. **Integrate**: Use the extension in your applications
4. **Customize**: Modify false positive rates and expected counts for your use case

---

If you find this project helpful and would like to support its development, consider buying me a coffee:

[![Buy Me A Coffee](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://buymeacoffee.com/amal_vs)

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

