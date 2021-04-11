#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>

#define ERROR -1
#define STOP_LINE 0
#define TABLE_SIZE 256
#define CONSOLE_INPUT_SIZE 30
#define DECIMAL 0
#define BUFFER_SIZE 1024

long long readNum()
{
    long long numberOfTheLine;
    char* pVoid = NULL;
    char console_input[CONSOLE_INPUT_SIZE];

    if(printf("Enter number of the line: ") == ERROR){
        perror("Error in printing text for user");
        return ERROR;
    }
    if (read(STDIN_FILENO, console_input, CONSOLE_INPUT_SIZE) == ERROR){
        perror("Problems with reading your text");
        return ERROR;
    }

    numberOfTheLine = strtoll(console_input, &pVoid, DECIMAL);

    return numberOfTheLine;
}


int fillTable(int fileDescriptor, size_t* lineLengths, off_t* fileOffsets){
    char readBuffer[BUFFER_SIZE];
    int actualBufferSize =  BUFFER_SIZE;
    int currentLineIndex = 1;
    int fileOffsetIndex = 1;
    int currentOffset = 0;

    while (actualBufferSize > 0){
        actualBufferSize = read(fileDescriptor, readBuffer, BUFFER_SIZE);
        if (actualBufferSize == ERROR){
            perror("Can't read current text");
            return ERROR;
        }

        for (size_t i = 0; i < actualBufferSize; i++){
            lineLengths[currentLineIndex]++;
            currentOffset++;
            if (readBuffer[i] == '\n'){
                fileOffsets[fileOffsetIndex] = (off_t)(currentOffset - lineLengths[currentLineIndex]);
                currentLineIndex++;
                fileOffsetIndex++;
            }
        }

    }
    return (currentLineIndex);
}

int readLineFromFile(int fileDescriptor, int lineLength, off_t offset, char* line){

    off_t lseekCheck = lseek(fileDescriptor, offset, SEEK_SET);
    if (lseekCheck == ERROR){
        perror("Seek error");
        return ERROR;
    }

    int readCheck = read(fileDescriptor, line, lineLength);
    if (readCheck == ERROR){
        perror("Can not read current text");
        return ERROR;
    }

    return EXIT_SUCCESS;
}

int printLine(char* line, int line_length){
    int writeCheck = write(STDOUT_FILENO, line, line_length);
    if (writeCheck == ERROR){
        perror("Can't print line");
        return ERROR;
    }

    return EXIT_SUCCESS;
}

int getLines(int file_descriptor, size_t* line_lengths, off_t* file_offsets, int number_of_lines){
    off_t offset = 0;
    int lineLength = 0;
    long long lineNumber = 0;

    while(true){
        lineNumber = readNum();

        if (lineNumber == ERROR){
            printf("%s", "Can't get number from user");
            return ERROR;
        }

        if (lineNumber == LLONG_MAX || lineNumber == LLONG_MIN){
            perror("Invalid line number");
            continue;
        }

        if (lineNumber == STOP_LINE)
            break;

        if (lineNumber > number_of_lines || lineNumber < 0){
            printf("%s", "Invalid line number: No such line in file\n");
            continue;
        }

        if (line_lengths[lineNumber] != 0){
            offset = file_offsets[lineNumber];
            lineLength = line_lengths[lineNumber];
            char line[lineLength];

            int readLineCheck = 0;
            readLineCheck = readLineFromFile(file_descriptor, lineLength, offset, line);

            if (readLineCheck == ERROR){
                printf("%s", "Can't read line");
                return ERROR;
            }

            int printLineCheck = 0;
            printLineCheck = printLine(line, lineLength);
            if (printLineCheck == ERROR){
                printf("%s", "Can't print line");
                return ERROR;
            }
        }
        printf("\n");
    }
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]){

    int fileDescriptor = 0;
    int linesNumber = 0;
    off_t fileOffsets[TABLE_SIZE]  = {0};
    size_t lineLengths[TABLE_SIZE]  = {0};

    /*
     * int open(const char *pathname, int flags); открывает файл pathname. Если файл не существует,
     * то есть возможность (если O_CREAT указан как флаг) его создания
     * Возвращаемое значение - файловый дискриптор (неотрицательный int)
     * Если системный вызов выполнени успешно, то возвращается наименьший файловый дискриптор, еще не открытый процессом
     * В случае ошибки возвращает -1
     * */

    fileDescriptor = open("text.txt", O_RDONLY); /* open for reading only */

    if (fileDescriptor == ERROR){
        perror("File can not be opened \n");
        return EXIT_FAILURE;
    }

    linesNumber = fillTable(fileDescriptor, lineLengths, fileOffsets);

    if (linesNumber == ERROR){
        printf("Error with filling the table");
        return EXIT_FAILURE;
    }

    int getLinesCheck = 0;

    getLinesCheck = getLines(fileDescriptor, lineLengths, fileOffsets, linesNumber);

    if (getLinesCheck == ERROR){
        printf("Error with printing lines");
        return EXIT_FAILURE;
    }

    int closeCheck = 0;
    closeCheck = close(fileDescriptor);

    if (closeCheck == ERROR){
        perror("Error with closing the file");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}