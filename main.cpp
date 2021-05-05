#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

extern int errno;

#define ERROR -1

#define SUCCESS 0
#define SUCCESS_GET_LINE_NUMBER 1
#define NO_ERROR 0

#define STRING_EQUAL 0
#define INVALID_LINE_NUMBER_INPUT 0
#define READ_EOF 0
#define TABLE_INIT_SIZE 100
#define READ_INIT -1
#define INPUT_SIZE 128
#define TRUE 1
#define STOP_INPUT 0
#define DECIMAL_SYSTEM 10
#define WITH_NEW_LINE 1
#define WITHOUT_NEW_LINE 0

typedef struct line_info {
    off_t offset;
    size_t length;
} line_info;

int add_to_table(line_info **table, long long *table_size, long long *table_length, off_t line_offset, size_t line_length) {
    if (table == NULL || *table == NULL || table_size == NULL || table_length == NULL) {
        fprintf(stderr, "Can't add element to table: Invalid argument(s)");
        return ERROR;
    }

    if (*table_length == *table_size) {
        line_info *ptr = (line_info *)realloc(*table, 2 * (*table_size) * sizeof(line_info));
        if (ptr == NULL) {
            perror("Can't create table");
            return ERROR;
        }
        *table = ptr;
        (*table_size) *= 2;
    }

    line_info elem;
    elem.offset = line_offset;
    elem.length = line_length;

    (*table)[*table_length] = elem;
    (*table_length)++;

    return SUCCESS;
}

line_info *create_table(int fildes, long long *table_length) {
    if (table_length == NULL) {
        fprintf(stderr, "Can't create table: Invalid argument\n");
        return NULL;
    }

    long long size = TABLE_INIT_SIZE;
    size_t line_length = 0, read_length = 1;
    off_t line_offset = 0;
    ssize_t read_check = READ_INIT;
    char c;


    *table_length = 0;
    line_info *table = (line_info *) malloc(size * sizeof(line_info));
    if (table == NULL) {
        perror("Can't create table");
        return NULL;
    }

    line_offset = lseek(fildes, 0L, SEEK_SET);
    if (line_offset == ERROR) {
        perror("Can't get/set position in file");
        free(table);
        return NULL;
    }

    while (read_check != READ_EOF) {
        read_check = read(fildes, &c, read_length);
        if (read_check == ERROR) {
            perror("Can't read from file");
            free(table);
            return NULL;
        }
        if (read_check != READ_EOF && c != '\n') {
            line_length++;
            continue;
        }

        int add_check = add_to_table(&table, &size, table_length, line_offset, line_length);
        if (add_check == ERROR) {
            free(table);
            return NULL;
        }

        line_length = 0;
        line_offset = lseek(fildes, 0L, SEEK_CUR);
        if (line_offset == ERROR) {
            perror("Can't get/set position in file");
            free(table);
            return NULL;
        }
    }
    return table;
}

int write_to_console(const char *buf, size_t length, int new_line) {
    ssize_t write_check = write(STDOUT_FILENO, buf, length);
    if (write_check == ERROR) {
        perror("Can't write to console");
        return ERROR;
    }
    if (new_line == WITH_NEW_LINE) {
        return write_to_console("\n", 1, WITHOUT_NEW_LINE);
    }
    return SUCCESS;
}

int get_line_number(long long *line_num) {
    char input[INPUT_SIZE + 1];

    int write_check = write_to_console("Enter line number: ", 19, WITHOUT_NEW_LINE);
    if (write_check == ERROR) {
        return ERROR;
    }

    ssize_t bytes_read = read(STDIN_FILENO, input, INPUT_SIZE);
    if (bytes_read == ERROR) {
        perror("Can't get line number");
        return ERROR;
    }
    if (bytes_read == 0) {
        return INVALID_LINE_NUMBER_INPUT;
    }
    input[bytes_read] = '\0';

    char *endptr = input;
    *line_num = strtoll(input, &endptr, DECIMAL_SYSTEM);

    if (errno != NO_ERROR) {
        perror("Can't convert given number");
        errno = 0;
        return INVALID_LINE_NUMBER_INPUT;
    }

    int compare1_result = strcmp(endptr, "\n");
    int compare2_result = strcmp(endptr, "");
    if (compare1_result != STRING_EQUAL && compare2_result != STRING_EQUAL) {
        fprintf(stderr, "Number contains invalid symbols\n");
        return INVALID_LINE_NUMBER_INPUT;
    }

    return SUCCESS_GET_LINE_NUMBER;
}

int read_line(int fildes, off_t offset, size_t length, char *buf) {
    off_t lseek_check = lseek(fildes, offset, SEEK_SET);
    if (lseek_check == ERROR) {
        perror("Can't get/set position in file");
        return ERROR;
    }

    ssize_t read_check = read(fildes, buf, length);
    if (read_check == ERROR) {
        perror("Can't read from file");
        return ERROR;
    }

    return SUCCESS;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 0;
    }

    int fildes = open(argv[1], O_RDONLY);
    if (fildes == ERROR) {
        perror("Can't open file");
        return 0;
    }

    long long table_length = 0, line_num = 0;
    line_info *table = create_table(fildes, &table_length);
    if (table != NULL) {
        while(TRUE) {
            int get_line_num_check = get_line_number(&line_num);

            if (get_line_num_check == ERROR) {
                break;
            }

            if (get_line_num_check == INVALID_LINE_NUMBER_INPUT) {
                continue;
            }

            if (line_num < 0 || line_num > table_length) {
                fprintf(stderr, "Invalid line number. It has to be in range [0, %lld]\n", table_length);
                continue;
            }

            if (line_num == STOP_INPUT) {
                break;
            }

            off_t line_offset = table[line_num - 1].offset;
            size_t line_length = table[line_num - 1].length;
            char line[line_length];

            int read_check = read_line(fildes, line_offset, line_length, line);
            if (read_check == ERROR) {
                break;
            }

            int write_check = write_to_console(line, line_length, WITH_NEW_LINE);
            if (write_check == ERROR) {
                break;
            }
        }

        free(table);
    }

    int close_check = close(fildes);
    if (close_check == ERROR) {
        perror("Can't close file");
    }
    return 0;
}