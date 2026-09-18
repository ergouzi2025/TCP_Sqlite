# TCP_Sqlite

A lightweight TCP gateway built in C that receives sensor data from clients, validates and parses commands, and stores them in SQLite with WAL mode enabled.

The project is designed for simple IoT or embedded-style sensor ingestion over TCP, with a clean separation between network handling, protocol parsing, services, and persistent storage.

## Features

- TCP server for client connections
- Command parser for sensor and query operations
- SQLite-backed data persistence
- Automatic schema initialization
- WAL mode enabled for SQLite (`PRAGMA journal_mode=WAL;`)
- Batch query support for record retrieval
- Modular project structure for easier extension

## Architecture

The project is organized as a small layered server:

- `app/` — entry point and main application flow
- `core/` — gateway logic that orchestrates requests
- `protocol/tcp/` — TCP command parsing and protocol handling
- `service/` — business logic for sensor data and queries
- `storage/sqlite/` — SQLite database wrapper and schema operations
- `transport/tcp/` — socket server implementation
- `config/` — global compile-time configuration
- `utils/log/` — logging utilities

## Project Structure

//text
TCP_Sqlite/
├── app/
│   └── main.c
├── config/
│   └── config.h
├── core/
│   ├── gateway.c
│   └── gateway.h
├── protocol/
│   └── tcp/
│       ├── tcp_parser.c
│       └── tcp_parser.h
├── service/
│   ├── data_service.c
│   ├── data_service.h
│   ├── sensor_service.c
│   └── sensor_service.h
├── storage/
│   └── sqlite/
│       ├── sqlite_db.c
│       └── sqlite_db.h
├── transport/
│   └── tcp/
│       ├── tcp_server.c
│       └── tcp_server.h
├── utils/
│   └── log/
│       ├── log.c
│       └── log.h
├── sqlite_arm_lib/
│   └── sqlite3-arm/
├── Makefile
├── Makefile.arm
├── README.md
└── tcp_sqlite_server
```

## Requirements

- Linux environment
- GCC
- SQLite development library
- pthread support

The project is built with the following default compile settings:

```bash
gcc -Wall -Wextra -std=c11 -I. -c ...
```

and links against:

```bash
-lsqlite3 -lpthread
```

## Build

From the project root:

```bash
make
```

This produces the executable:

```bash
./tcp_sqlite_server
```

## Run

The server expects two command-line arguments:

```bash
./tcp_sqlite_server <ip> <port>
```

Example:

```bash
./tcp_sqlite_server 127.0.0.1 19000
```

## Configuration

Database path and buffer settings are defined in [config/config.h](config/config.h):

```c
#ifndef CONFIG_H
#define CONFIG_H

#define BUFFER_SIZE     128
#define DB_FILE         "storage/sqlite/data.db"
#define TCP_BACKLOG     5

#endif
```

This means:

- the SQLite database file is stored at `storage/sqlite/data.db`
- SQLite WAL files are created alongside it:
  - `storage/sqlite/data.db-shm`
  - `storage/sqlite/data.db-wal`

> Note: `DB_FILE` is a relative path. It is resolved relative to the process working directory. To keep the database in the project directory, run the server from the project root or ensure the working directory matches the expected location.

## Supported Commands

The gateway processes command strings over TCP using comma-separated values.

### 1) SENSOR

Store sensor values for a device.

```text
SENSOR,<device_id>,<data1>,<data2>,<data3>,<data4>
```

Example:

```text
SENSOR,DEV2,26.8,55.2,101.3,568.4
```

### 2) QUERY

Retrieve data records in a range.

```text
QUERY,<start_id>,<end_id>
```

Example:

```text
QUERY,1,20
```

### 3) HELP

Display help and command usage.

```text
HELP
```

The server responds with command explanations and the current ID range.

## Database Schema

The SQLite database is initialized automatically when the server opens the database file. The schema includes the following table:

```sql
CREATE TABLE IF NOT EXISTS sensor_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id TEXT NOT NULL,
    data_1 REAL,
    data_2 REAL,
    data_3 REAL,
    data_4 REAL,
    timestamp DATETIME DEFAULT (datetime('now', 'localtime'))
);

CREATE INDEX IF NOT EXISTS idx_sensor_data_device_id
ON sensor_data(device_id);
```

## Example Workflow

1. Start the server:

```bash
./tcp_sqlite_server 127.0.0.1 19000
```

2. Connect via TCP client, for example with `nc`:

```bash
nc 127.0.0.1 19000
```

3. Send data:

```text
SENSOR,DEV2,26.8,55.2,101.3,568.4
```

4. Query records:

```text
QUERY,1,20
```

5. Request help:

```text
HELP
```

## Logging

The project uses a simple custom logger located in `utils/log/`. It prints startup, data storage, and operation events to stdout/stderr, which makes debugging easier during development and deployment.

## Notes

- WAL mode is enabled to improve read/write behavior for concurrent access.
- The database connection is opened by `sqlite_db_open(DB_FILE)` from the application entry point.
- The gateway architecture keeps the TCP layer separate from the storage layer, making the project easier to extend for additional protocols or sensor types.

## License

This project is provided for educational and development purposes. Please add an appropriate license file if you plan to publish it publicly on GitHub.

For example, if you want to use MIT licensing, you can add:

```bash
mit
```

or create a LICENSE file with the MIT text before publishing the repository.

## Contributing

Contributions, issues, and feature requests are welcome. If you plan to open-source the project on GitHub, a typical workflow is:

```bash
git init
git add .
git commit -m "Initial commit"
git branch -M main
git remote add origin <your-repository-url>
git push -u origin main
```

## Summary

TCP_Sqlite is a compact C-based TCP sensor gateway that demonstrates how to:

- accept socket connections,
- parse command-based requests,
- validate and process sensor readings,
- persist data in SQLite,
- and expose a simple protocol for monitoring and retrieval.

It is suitable as a practical example of combining low-level networking with embedded-friendly database persistence in a single project.