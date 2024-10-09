# ESQL - Simple SQL-Like Database Engine

ESQL is a minimalist database engine that allows basic SQL-like commands for database creation, table management, data insertion, and running `SELECT` queries with conditions. This project includes a client/server architecture and a command-line interface.

## Features

- **Create Databases**: Use the `CREATE DATABASE` command to create a new database.
- **Create Tables**: Define tables with columns of type `varchar` and `int`.
- **Insert Data**: Use the `INSERT INTO` command to insert data into tables.
- **Run Queries**: Perform `SELECT` queries to filter and retrieve data with `WHERE`, `AND`, and `OR` conditions.
- **Show Databases and Tables**: List available databases and tables within a selected database.

## Available Commands

### SQL

- `CREATE DATABASE <db_name>;`
- `USE <db_name>;`
- `CREATE TABLE <table_name> (column1:type1, column2:type2, ...);`
- `INSERT INTO <table_name> VALUES (value1, value2, ...);`
- `SELECT * FROM <table_name> [WHERE condition];`
- `SHOW DATABASES;`
- `SHOW TABLES;`
- `SHOW COLUMNS FROM <table_name>;`

### Example Commands

```sql
CREATE DATABASE boutique;
USE boutique;
CREATE TABLE alcohol (name:varchar(50), percentage:int, color:varchar(20), price:int);
INSERT INTO alcohol VALUES ('vodka', 40, 'transparent', 20);
INSERT INTO alcohol VALUES ('whiskey', 45, 'amber', 25);
SELECT * FROM alcohol WHERE price <= 20 AND percentage > 10;
```

## Project Structure

- `src/main.c`: Contains the core logic for the application, including functions for handling databases, tables, and queries.
- `Makefile`: Makefile to compile and run the project.
- `example.sql`: Sample SQL file with commands to execute.

## Compilation and Execution

### Prerequisites

- **GCC**: A C compiler is required to build the project.

### Compilation

To compile the project, run the following command from the project’s root directory:

```bash
make
```

### Running the Application

#### Server

To start the server, run the following command after compiling:

```bash
./out/esql -s
```

#### Client

To interact with the server, run the following command in another terminal:

```bash
./out/esql -u
```

#### Execute from SQL File

You can also execute SQL commands from a `.sql` file:

```bash
./out/esql -f example.sql
```

### Installation

To install ESQL globally on your system, run:

```bash
sudo make install
```

This will install the executable in `/usr/local/bin`.

### Cleaning Up

To clean up the compiled files, run:

```bash
make clean
```

## Contributing

Contributions are welcome! Feel free to submit issues or pull requests to improve this project.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for more details.
