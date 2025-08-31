# PostgreSQL extension makefile using PGXS

MODULE_big = octo_bloom
OBJS = src/octo_bloom.o src/bloom_filter.o src/shared_memory.o src/trigger_manager.o src/background_worker.o

EXTENSION = octo_bloom
DATA = sql/octo_bloom--1.0.sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

# Additional compiler flags
override CXXFLAGS += -std=c++17 -fPIC -fno-exceptions -fno-rtti
override CFLAGS += -fPIC

# Include paths
override CXXFLAGS += -I$(shell $(PG_CONFIG) --includedir-server)
override CFLAGS += -I$(shell $(PG_CONFIG) --includedir-server)

# Link OpenSSL
SHLIB_LINK += $(shell pkg-config --libs openssl)