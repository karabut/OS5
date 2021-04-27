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

    //буффер для считывания
    char readBuffer[BUFFER_SIZE];

    int numberOfReadElements =  BUFFER_SIZE;
    int currentLineIndex = 1;
    int fileOffsetIndex = 1;
    int currentOffset = 0;

    while (numberOfReadElements > 0){

        //сколько считали из файла (максимально buffer size, если столько в файле есть)
        numberOfReadElements = read(fileDescriptor, readBuffer, BUFFER_SIZE);

        //проверка на ошибку read
        if (numberOfReadElements == ERROR){
            perror("Can't read current text");
            return ERROR;
        }

        //цикл по количеству считанных из файла элементов
        for (size_t i = 0; i < numberOfReadElements; i++){

            //добавляем к длине строки по +1
            lineLengths[currentLineIndex]++;

            //добавляем к сдвигу по +1
            currentOffset++;

            //если элемент - переход на новую строку
            if (readBuffer[i] == '\n'){

                fileOffsets[fileOffsetIndex] = (off_t)(currentOffset - lineLengths[currentLineIndex]);

                currentLineIndex++;

                fileOffsetIndex++;

            }
        }

    }

    //возвращаем количество строк
    return (currentLineIndex);
}

int readLineFromFile(int fileDescriptor, int lineLength, off_t offset, char* line){

    //устанавливаем смещение от начала файла
    off_t lseekCheck = lseek(fileDescriptor, offset, SEEK_SET);
    if (lseekCheck == ERROR){
        perror("Seek error");
        return ERROR;
    }

    //считываем нужную строку из файла
    int readCheck = read(fileDescriptor, line, lineLength);

    if (readCheck == ERROR){
        perror("Can not read current text");
        return ERROR;
    }

    return EXIT_SUCCESS;
}

int printLine(char* line, int lineLength){
    int writeCheck = write(STDOUT_FILENO, line, lineLength);
    if (writeCheck == ERROR){
        perror("Can't print line");
        return ERROR;
    }

    return EXIT_SUCCESS;
}

int getLines(int fileDescriptor, size_t* lineLengths, off_t* fileOffsets, int numberOfLines){
    off_t offset = 0;
    int lineLength = 0;
    long long lineNumber = 0;

    while(true){
        //пользователь вводит нужную строку
        lineNumber = readNum();

        if (lineNumber == ERROR){
            printf("%s", "Can't get number from user");
            return ERROR;
        }

        //если введенное число слишком большое или наборот
        if (lineNumber == LLONG_MAX || lineNumber == LLONG_MIN){
            perror("Invalid line number");
            continue;
        }

        //если пользователь ввел 0
        if (lineNumber == STOP_LINE)
            break;

        //если такой строки нет или пользователь ввел отрицательно значение
        if (lineNumber > numberOfLines || lineNumber < 0){
            printf("%s", "Invalid line number: No such line in file\n");
            continue;
        }

        //если строка не нулевая
        if (lineLengths[lineNumber] != 0){
            offset = fileOffsets[lineNumber];
            lineLength = lineLengths[lineNumber];
            //создаем буфер под размер строки
            char line[lineLength];


            int readLineCheck = 0;
            //считываем строку из файла в массив line
            readLineCheck = readLineFromFile(fileDescriptor, lineLength, offset, line);

            if (readLineCheck == ERROR){
                printf("%s", "Can't read line");
                return ERROR;
            }

            int printLineCheck = 0;
            //печатаем поолученную строку
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

    //инициализируем файловый дискриптор...таблицу сдвигов и длин строк
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

    //проверка открылся ли файл
    if (fileDescriptor == ERROR){
        perror("File can not be opened \n");
        return EXIT_FAILURE;
    }

    //получаем количество строк в файле и параллельно строим таблицы
    linesNumber = fillTable(fileDescriptor, lineLengths, fileOffsets);

    if (linesNumber == ERROR){
        printf("Error with filling the table");
        return EXIT_FAILURE;
    }


    int getLinesCheck = 0;
    //от пользователя получаем нужную строку
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