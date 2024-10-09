#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>

#define PORT 8080
#define MAX_COLUMNS 100
#define BUFFER_SIZE 1024
#define BASE_DB_PATH "databases/"
#define MAX_CONDITIONS 10

typedef struct {
    char column_name[BUFFER_SIZE];
    char operator[3];
    char value[BUFFER_SIZE];
    char logical_op[4];
} Condition;


char current_db[BUFFER_SIZE] = "";

void trim_whitespace(char *str) {
    char *start = str;
    while(isspace((unsigned char)*start)) start++;

    if(*start == 0) {
        *str = 0;
        return;
    }

    char *end = start + strlen(start) - 1;
    while(end > start && isspace((unsigned char)*end)) end--;

    *(end+1) = '\0';

    if (start != str) {
        memmove(str, start, end - start + 2);
    }
}

int parse_conditions(const char *where_clause, Condition conditions[], int *condition_count) {
    char clause_copy[BUFFER_SIZE];
    strncpy(clause_copy, where_clause, BUFFER_SIZE);

    char *token = strtok(clause_copy, " ");
    int idx = 0;

    while (token != NULL && idx < MAX_CONDITIONS) {
        strncpy(conditions[idx].column_name, token, BUFFER_SIZE);

        token = strtok(NULL, " ");
        if (token == NULL) return -1;
        strncpy(conditions[idx].operator, token, 3);

        token = strtok(NULL, " ");
        if (token == NULL) return -1;
        strncpy(conditions[idx].value, token, BUFFER_SIZE);

        token = strtok(NULL, " ");
        if (token != NULL && (strcmp(token, "AND") == 0 || strcmp(token, "OR") == 0)) {
            strncpy(conditions[idx].logical_op, token, 4);
            token = strtok(NULL, " ");
        } else {
            conditions[idx].logical_op[0] = '\0';
        }

        idx++;
    }

    *condition_count = idx;
    return 0;
}

void create_directory_if_not_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0700);
    }
}

void create_database(const char *db_name, char *response) {
    char db_path[BUFFER_SIZE];
    snprintf(db_path, BUFFER_SIZE, "%s%s", BASE_DB_PATH, db_name);
    create_directory_if_not_exists(db_path);
    snprintf(response, BUFFER_SIZE, "Database '%s' created.", db_name);
}

void show_databases(char *response) {
    DIR *d;
    struct dirent *dir;
    d = opendir(BASE_DB_PATH);
    if (!d) {
        snprintf(response, BUFFER_SIZE, "Error opening databases directory.");
        return;
    }

    strcpy(response, "Databases:\n");
    while ((dir = readdir(d)) != NULL) {
        struct stat st;
        char path[BUFFER_SIZE];
        snprintf(path, sizeof(path), "%s%s", BASE_DB_PATH, dir->d_name);
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode) && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
            strcat(response, dir->d_name);
            strcat(response, "\n");
        }
    }
    closedir(d);
}

void use_database(const char *db_name, char *response) {
    strcpy(current_db, db_name);
    snprintf(response, BUFFER_SIZE, "Using database: %s", db_name);
}

void create_table(const char *table_name, const char *fields, char *response) {
    if (strlen(current_db) == 0) {
        snprintf(response, BUFFER_SIZE, "No database selected. Use `USE <db_name>`.");
        return;
    }

    if (!strchr(fields, ':')) {
        snprintf(response, BUFFER_SIZE, "Invalid column definition. Use the format 'name:type'.");
        return;
    }

    char table_path[BUFFER_SIZE];
    snprintf(table_path, sizeof(table_path), "%s%s/%s.bin", BASE_DB_PATH, current_db, table_name);

    FILE *file = fopen(table_path, "wb");
    if (file) {
        size_t fields_len = strlen(fields);
        if (fwrite(fields, sizeof(char), fields_len, file) != fields_len) {
            snprintf(response, BUFFER_SIZE, "Error writing fields to table '%s'.", table_name);
            fclose(file);
            return;
        }

        fputc('\n', file);

        printf("Fields : %s\n", fields);
        char *token = strtok(fields, ",");
        while (token != NULL) {
            printf("Token : %s\n", token);
            char column_name[BUFFER_SIZE], column_type[BUFFER_SIZE];
            sscanf(token, "%[^:]:%s", column_name, column_type);
            printf("Column name : %s\n", column_name);
            printf("Column type : %s\n", column_type);
            token = strtok(NULL, ",");
        }

        printf("Table '%s' created in database '%s'.\n", table_name, current_db);

        snprintf(response, BUFFER_SIZE, "Table '%s' created in database '%s'.", table_name, current_db);
        fclose(file);
    } else {
        snprintf(response, BUFFER_SIZE, "Error creating table '%s'.", table_name);
    }
}

void read_table_columns(const char *table_name, char *columns, char *response) {
    if (strlen(current_db) == 0) {
        snprintf(response, BUFFER_SIZE, "No database selected. Use `USE <db_name>`.");
        return;
    }

    char table_path[BUFFER_SIZE];
    snprintf(table_path, sizeof(table_path), "%s%s/%s.bin", BASE_DB_PATH, current_db, table_name);

    FILE *file = fopen(table_path, "rb");
    if (file) {
        if (fgets(columns, BUFFER_SIZE, file) != NULL) {
            size_t len = strlen(columns);
            if (len > 0 && columns[len - 1] == '\n') {
                columns[len - 1] = '\0';
            }

            snprintf(response, BUFFER_SIZE, "Columns of table '%s': %s", table_name, columns);
            
        } else {
            snprintf(response, BUFFER_SIZE, "Error reading columns from table '%s'.", table_name);
        }
        fclose(file);
    } else {
        snprintf(response, BUFFER_SIZE, "Error opening table '%s'.", table_name);
    }
}

void show_tables(char *response) {
    if (strlen(current_db) == 0) {
        snprintf(response, BUFFER_SIZE, "No database selected. Use `USE <db_name>`.");
        return;
    }

    char db_path[BUFFER_SIZE];
    snprintf(db_path, sizeof(db_path), "%s%s", BASE_DB_PATH, current_db);

    DIR *d;
    struct dirent *dir;
    d = opendir(db_path);
    if (!d) {
        snprintf(response, BUFFER_SIZE, "Error opening database '%s'.", current_db);
        return;
    }

    strcpy(response, "Tables:\n");
    while ((dir = readdir(d)) != NULL) {
        struct stat st;
        char file_path[BUFFER_SIZE];
        snprintf(file_path, BUFFER_SIZE, "%s/%s", db_path, dir->d_name);
        if (stat(file_path, &st) == 0 && S_ISREG(st.st_mode) && strstr(dir->d_name, ".bin")) {
            dir->d_name[strlen(dir->d_name) - 4] = '\0'; 
            strcat(response, dir->d_name);
            strcat(response, "\n");
        }
    }
    closedir(d);
}


void insert_into_table(const char *table_name, char *columns[], char *values[], int column_count, char *response) {
    if (strlen(current_db) == 0) {
        snprintf(response, BUFFER_SIZE, "No database selected. Use `USE <db_name>`.");
        return;
    }

    char table_path[BUFFER_SIZE];
    snprintf(table_path, sizeof(table_path), "%s%s/%s.bin", BASE_DB_PATH, current_db, table_name);

    FILE *file = fopen(table_path, "ab+");
    if (!file) {
        snprintf(response, BUFFER_SIZE, "Error opening table '%s' for insertion.", table_name);
        return;
    }

    fseek(file, 0, SEEK_END);

    for (int i = 0; i < column_count; i++) {
        char column_name[BUFFER_SIZE], column_type[BUFFER_SIZE];
        sscanf(columns[i], "%[^:]:%s", column_name, column_type);

        if (strcmp(column_type, "int") == 0) {
            int value = atoi(values[i]);
            fwrite(&value, sizeof(int), 1, file);
        }
        else if (strstr(column_type, "varchar") != NULL) {
            unsigned char value_len = strlen(values[i]);
            fwrite(&value_len, sizeof(unsigned char), 1, file);
            fwrite(values[i], sizeof(char), value_len, file);
        } else {
            snprintf(response, BUFFER_SIZE, "Unsupported column type: %s", column_type);
            fclose(file);
            return;
        }
    }

    snprintf(response, BUFFER_SIZE, "Inserted values into table '%s' successfully.", table_name);
    fclose(file);
}


int evaluate_condition(const char *value, Condition condition, const char *column_type) {
    if (strcmp(column_type, "int") == 0) {
        int int_value = atoi(value);
        int int_condition_value = atoi(condition.value);

        if (strcmp(condition.operator, "=") == 0) {
            return int_value == int_condition_value;
        } else if (strcmp(condition.operator, ">") == 0) {
            return int_value > int_condition_value;
        } else if (strcmp(condition.operator, "<") == 0) {
            return int_value < int_condition_value;
        } else if (strcmp(condition.operator, ">=") == 0) {
            return int_value >= int_condition_value;
        } else if (strcmp(condition.operator, "<=") == 0) {
            return int_value <= int_condition_value;
        } else {
            return 0;
        }
    } else if (strncmp(column_type, "varchar", 7) == 0) {
        if (strcmp(condition.operator, "=") == 0) {
            return strcmp(value, condition.value) == 0;
        } else {
            return 0;
        }
    }
    return 0;
}

void select_from_table(const char *table_name, const char *where_clause, char *output) {
    if (strlen(current_db) == 0) {
        snprintf(output, BUFFER_SIZE, "No database selected. Use `USE <db_name>`.");
        return;
    }

    char table_path[BUFFER_SIZE];
    snprintf(table_path, sizeof(table_path), "%s%s/%s.bin", BASE_DB_PATH, current_db, table_name);

    FILE *file = fopen(table_path, "rb");
    if (!file) {
        snprintf(output, BUFFER_SIZE, "Error opening table '%s'.", table_name);
        return;
    }

    char columns_line[BUFFER_SIZE];
    if (fgets(columns_line, sizeof(columns_line), file) == NULL) {
        snprintf(output, BUFFER_SIZE, "Error reading columns from table '%s'.", table_name);
        fclose(file);
        return;
    }

    size_t len = strlen(columns_line);
    if (len > 0 && columns_line[len - 1] == '\n') {
        columns_line[len - 1] = '\0';
    }

    char *column_names[MAX_COLUMNS];
    char *column_types[MAX_COLUMNS];
    int column_count = 0;

    char *columns_line_copy = strdup(columns_line);
    char *column_token = strtok(columns_line_copy, ",");
    while (column_token != NULL && column_count < MAX_COLUMNS) {
        char *colon_pos = strchr(column_token, ':');
        if (colon_pos != NULL) {
            *colon_pos = '\0';
            column_names[column_count] = strdup(column_token);
            column_types[column_count] = strdup(colon_pos + 1);
            column_count++;
        } else {
            snprintf(output, BUFFER_SIZE, "Invalid column definition in table '%s'.", table_name);
            fclose(file);
            free(columns_line_copy);
            return;
        }
        column_token = strtok(NULL, ",");
    }
    free(columns_line_copy);

    Condition conditions[MAX_CONDITIONS];
    int condition_count = 0;
    if (where_clause != NULL && strlen(where_clause) > 0) {
        if (parse_conditions(where_clause, conditions, &condition_count) != 0) {
            snprintf(output, BUFFER_SIZE, "Error parsing WHERE conditions.");
            fclose(file);
            for (int i = 0; i < column_count; i++) {
                free(column_names[i]);
                free(column_types[i]);
            }
            return;
        }
    }

    strcpy(output, "");
    strcat(output, "Columns:\n");
    for (int i = 0; i < column_count; i++) {
        strcat(output, column_names[i]);
        strcat(output, "\t");
    }
    strcat(output, "\nData:\n");

    while (1) {
        char row_values[MAX_COLUMNS][BUFFER_SIZE];
        int end_of_file = 0;

        for (int i = 0; i < column_count; i++) {
            if (strcmp(column_types[i], "int") == 0) {
                int int_value;
                size_t read_size = fread(&int_value, sizeof(int), 1, file);
                if (read_size != 1) {
                    end_of_file = 1;
                    break;
                }
                snprintf(row_values[i], BUFFER_SIZE, "%d", int_value);
            } else if (strncmp(column_types[i], "varchar", 7) == 0) {
                unsigned char str_len;
                size_t read_size = fread(&str_len, sizeof(unsigned char), 1, file);
                if (read_size != 1) {
                    end_of_file = 1;
                    break;
                }
                char str_value[256];
                read_size = fread(str_value, sizeof(char), str_len, file);
                if (read_size != str_len) {
                    end_of_file = 1;
                    break;
                }
                str_value[str_len] = '\0';
                strncpy(row_values[i], str_value, BUFFER_SIZE);
            } else {
                snprintf(output, BUFFER_SIZE, "Unsupported column type '%s' in table '%s'.", column_types[i], table_name);
                fclose(file);
                for (int i = 0; i < column_count; i++) {
                    free(column_names[i]);
                    free(column_types[i]);
                }
                return;
            }
        }
        if (end_of_file) {
            break;
        }

        int row_matches = 1; 
        for (int i = 0; i < condition_count; i++) {
            int col_idx = -1;
            for (int j = 0; j < column_count; j++) {
                if (strcmp(conditions[i].column_name, column_names[j]) == 0) {
                    col_idx = j;
                    break;
                }
            }

            if (col_idx == -1) {
                snprintf(output, BUFFER_SIZE, "Column '%s' not found in table '%s'.", conditions[i].column_name, table_name);
                fclose(file);
                for (int i = 0; i < column_count; i++) {
                    free(column_names[i]);
                    free(column_types[i]);
                }
                return;
            }

            int condition_result = evaluate_condition(row_values[col_idx], conditions[i], column_types[col_idx]);

            if (i > 0 && strcmp(conditions[i - 1].logical_op, "OR") == 0) {
                row_matches = row_matches || condition_result;
            } else if (i > 0 && strcmp(conditions[i - 1].logical_op, "AND") == 0) {
                row_matches = row_matches && condition_result;
            } else {
                row_matches = condition_result;
            }
        }

        if (row_matches) {
            for (int i = 0; i < column_count; i++) {
                strcat(output, row_values[i]);
                strcat(output, "\t");
            }
            strcat(output, "\n");
        }
    }

    fclose(file);
    for (int i = 0; i < column_count; i++) {
        free(column_names[i]);
        free(column_types[i]);
    }
}

int parse_values(const char *values_str, char *value_array[], int max_values) {
    int count = 0;
    const char *ptr = values_str;
    while (*ptr && count < max_values) {
        while (isspace(*ptr)) ptr++;
        if (*ptr == '\'' || *ptr == '\"') {
            char quote = *ptr++;
            const char *start = ptr;
            while (*ptr && *ptr != quote) ptr++;
            size_t len = ptr - start;
            value_array[count] = strndup(start, len);
            if (*ptr == quote) ptr++;
        } else {
            const char *start = ptr;
            while (*ptr && *ptr != ',' && !isspace(*ptr)) ptr++;
            size_t len = ptr - start;
            value_array[count] = strndup(start, len);
        }
        count++;
        while (isspace(*ptr)) ptr++; 
        if (*ptr == ',') ptr++;
    }
    return count;
}

void handle_command(const char *command, char *response) {
    char cmd_copy[BUFFER_SIZE];
    strncpy(cmd_copy, command, BUFFER_SIZE);
    cmd_copy[BUFFER_SIZE - 1] = '\0';

    trim_whitespace(cmd_copy);
    size_t len = strlen(cmd_copy);
    if (cmd_copy[len - 1] == ';') {
        cmd_copy[len - 1] = '\0';
        trim_whitespace(cmd_copy);
    }

    char action[BUFFER_SIZE];
    sscanf(cmd_copy, "%s", action);

    for (int i = 0; action[i]; i++) {
        action[i] = tolower((unsigned char)action[i]);
    }

    if (strcmp(action, "create") == 0) {
        char entity[BUFFER_SIZE], name[BUFFER_SIZE];
        if (sscanf(cmd_copy, "%*s %s %s", entity, name) == 2) {
            for (int i = 0; entity[i]; i++) {
                entity[i] = tolower((unsigned char)entity[i]);
            }
            if (strcmp(entity, "database") == 0) {
                create_database(name, response);
            } else if (strcmp(entity, "table") == 0) {
                if (strcasestr(cmd_copy, "table") && strcasestr(cmd_copy, "(") && strcasestr(cmd_copy, ")")) {
                    const char *columns_start = strchr(cmd_copy, '(');
                    const char *columns_end = columns_start;
                    int paren_count = 1;
                    while (*columns_end && paren_count > 0) {
                        columns_end++;
                        if (*columns_end == '(') paren_count++;
                        else if (*columns_end == ')') paren_count--;
                    }
                    if (paren_count != 0) {
                        snprintf(response, BUFFER_SIZE, "Mismatched parentheses in command.");
                        return;
                    }
                    
                    if (columns_start && columns_end && columns_end > columns_start) {
                        char columns[BUFFER_SIZE];
                        strncpy(columns, columns_start + 1, columns_end - columns_start - 1);
                        columns[columns_end - columns_start - 1] = '\0';
                        char table_name[BUFFER_SIZE];
                        sscanf(cmd_copy, "%*s %*s %s", table_name);
                        create_table(table_name, columns, response);
                    } else {
                        snprintf(response, BUFFER_SIZE, "Invalid CREATE command.");
                    }
                } else {
                    snprintf(response, BUFFER_SIZE, "Invalid CREATE command syntax.");
                }
            } else {
                snprintf(response, BUFFER_SIZE, "Invalid CREATE command.");
            }
        } else {
            snprintf(response, BUFFER_SIZE, "Invalid CREATE command syntax.");
        }
    } else if (strcmp(action, "use") == 0) {
        char db_name[BUFFER_SIZE];
        if (sscanf(cmd_copy, "%*s %s", db_name) == 1) {
            use_database(db_name, response);
        } else {
            snprintf(response, BUFFER_SIZE, "Invalid USE command syntax.");
        }
    } else if (strcmp(action, "insert") == 0) {
        char table_name[BUFFER_SIZE], name[BUFFER_SIZE];
        int age;

        char *into_pos = strcasestr(cmd_copy, "into ");
        char *values_pos = strcasestr(cmd_copy, "values");

        if (into_pos && values_pos) {
            into_pos += strlen("into ");
            char *table_name_end = strchr(into_pos, ' ');
            if (table_name_end) {
                *table_name_end = '\0';
                strcpy(table_name, into_pos);
            } else {
                snprintf(response, BUFFER_SIZE, "Invalid table name in INSERT command.");
                return;
            }

            char columns[BUFFER_SIZE];
            read_table_columns(table_name, columns, response);

            char *column_array[BUFFER_SIZE];
            int column_count = 0;
            char *column_token = strtok(columns, ",");
            while (column_token != NULL) {
                column_array[column_count++] = column_token;
                column_token = strtok(NULL, ",");
            }

            values_pos += strlen("values");
            char *values_start = strchr(values_pos, '(');
            char *values_end = strchr(values_pos, ')');
            if (values_start && values_end && values_end > values_start) {
                *values_end = '\0';
                values_start++;

                char *value_array[BUFFER_SIZE];
                int value_count = parse_values(values_start, value_array, BUFFER_SIZE);

                if (column_count != value_count) {
                    snprintf(response, BUFFER_SIZE, "Column count and value count do not match.");
                    return;
                }
                insert_into_table(table_name, column_array, value_array, column_count, response);

                for (int i = 0; i < value_count; i++) {
                    free(value_array[i]);
                }
            } else {
                snprintf(response, BUFFER_SIZE, "Invalid values syntax.");
            }
        } else {
            snprintf(response, BUFFER_SIZE, "Invalid values format.");
        }
    } else if (strcmp(action, "select") == 0) {
        char table_name[BUFFER_SIZE];
        char *where_clause = NULL;
        char columns[BUFFER_SIZE];

        char *from_ptr = strstr(cmd_copy, "FROM");
        if (from_ptr) {
            char *where_ptr = strstr(from_ptr, "WHERE");
            if (where_ptr) {
                sscanf(from_ptr, "FROM %s", table_name);
                where_clause = where_ptr + strlen("WHERE ");
            } else {
                sscanf(from_ptr, "FROM %s", table_name);
            }
            
            select_from_table(table_name, where_clause, response);
        } else {
            snprintf(response, BUFFER_SIZE, "Invalid SELECT command syntax.");
        }
    } else if (strcmp(action, "show") == 0) {
        char entity[BUFFER_SIZE];
        if (sscanf(cmd_copy, "%*s %s", entity) == 1) {
            for (int i = 0; entity[i]; i++) {
                entity[i] = tolower((unsigned char)entity[i]);
            }
            if (strcmp(entity, "databases") == 0) {
                show_databases(response);
            } else if (strcmp(entity, "tables") == 0) {
                show_tables(response);
            } else if (strcmp(entity, "columns") == 0) {
                char table_name[BUFFER_SIZE];
                if (sscanf(cmd_copy, "%*s %*s %s", table_name) == 1) {
                    char columns[BUFFER_SIZE];
                    read_table_columns(table_name, columns, response);
                } else {
                    snprintf(response, BUFFER_SIZE, "Invalid SHOW COLUMNS command syntax.");
                }
            } else {
                snprintf(response, BUFFER_SIZE, "Invalid SHOW command.");
            }
        } else {
            snprintf(response, BUFFER_SIZE, "Invalid SHOW command syntax.");
        }
    } else {
        snprintf(response, BUFFER_SIZE, "Invalid command.");
    }
}

void run_server() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    create_directory_if_not_exists(BASE_DB_PATH);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server waiting for connections on port %d...\n", PORT);

    while ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) >= 0) {
        memset(buffer, 0, BUFFER_SIZE); 
        int valread = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (valread <= 0) {
            close(client_socket);
            continue;
        }
        buffer[valread] = '\0'; 
        printf("Received command: %s\n", buffer);

        char response[BUFFER_SIZE * 10] = {0};
        handle_command(buffer, response);

        send(client_socket, response, strlen(response), 0);
        close(client_socket);
    }
}

void client() {
    while (1) {
        printf("$ ");
        char buffer[BUFFER_SIZE];
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        int sock = 0;
        struct sockaddr_in serv_addr;

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Socket creation error \n");
            return;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);

        if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
            printf("\nInvalid address/ Address not supported \n");
            return;
        }

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("\nConnection Failed \n");
            return;
        }

        send(sock, buffer, strlen(buffer), 0);

        char response[BUFFER_SIZE * 10] = {0};
        int valread = read(sock, response, sizeof(response) - 1);
        if (valread > 0) {
            response[valread] = '\0';
            printf("%s\n", response);
        }

        close(sock);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "-s") == 0) {
            run_server();
        } else if (strcmp(argv[1], "-u") == 0) {
            client();
        } else {
            fprintf(stderr, "Commandes : %s [-s | -u]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        run_server();
    }

    return 0;
}


