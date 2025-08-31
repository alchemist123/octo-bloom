FROM postgres:15-bookworm

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    postgresql-server-dev-15 \
    git \
    pkg-config \
    libssl-dev

# Copy extension source
COPY . /usr/src/octo-bloom
WORKDIR /usr/src/octo-bloom/build

# Build and install extension
RUN cmake .. && \
    make && \
    make install

# Clean up build dependencies
RUN apt-get purge -y build-essential cmake postgresql-server-dev-15 git pkg-config libssl-dev && \
    apt-get autoremove -y && \
    rm -rf /usr/src/octo-bloom

# Create extension on database initialization
COPY docker/initdb.d/ /docker-entrypoint-initdb.d/