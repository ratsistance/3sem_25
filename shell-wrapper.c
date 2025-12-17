#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 100    // Макс кол-во аргументов 
#define MAX_CMD 1024	// Макс длина строки

void execute(char **args, int input_fd, int output_fd) {
    pid_t pid = fork();	// Создание нового процесса
    if (pid == 0) {
        if (input_fd != 0) dup2(input_fd, 0), close(input_fd);
        if (output_fd != 1) dup2(output_fd, 1), close(output_fd);
        execvp(args[0], args);
        perror(args[0]);
        exit(1);
    }
}

int main() {
    char line[MAX_CMD];	// Буфер для ввода команды
    
    while (printf("> "), fgets(line, sizeof(line), stdin)) {	// Цикл продолжаетс пока читает строки
        line[strcspn(line, "\n")] = 0;
        if (!*line || !strcmp(line, "exit") || !strcmp(line, "quit")) break;	// Проверка на завершение
        
        char *cmds[MAX_ARGS], *cmd, *save;
        int cmd_count = 0, i, fd_in = 0, fd[2], status;
        
        // Разбиваем на команды по |
        for (cmd = strtok_r(line, "|", &save); cmd; cmd = strtok_r(NULL, "|", &save)) {
            while (*cmd == ' ') cmd++;
            cmds[cmd_count++] = cmd;
        }
        if (!cmd_count) continue;
        
        // Выполняем конвейер
        for (i = 0; i < cmd_count; i++) {
            char *args[MAX_ARGS], *arg;
            int arg_count = 0;
            
            // Разбиваем команду на аргументы
            for (arg = strtok(cmds[i], " "); arg; arg = strtok(NULL, " "))
                args[arg_count++] = arg;
            args[arg_count] = NULL;
            
            // Создаём pipe, если не последняя команда
            if (i < cmd_count - 1 && pipe(fd) < 0) {
                perror("pipe");
                break;
            }
            
            // Запускаем команду
            execute(args, fd_in, (i < cmd_count - 1) ? fd[1] : 1);
            
            // Закрываем ненужные дескрипторы
            if (fd_in != 0) close(fd_in);
            if (i < cmd_count - 1) close(fd[1]), fd_in = fd[0];
        }
        
        // Ждём последнюю команду и выводим код возврата
        while (wait(&status) > 0) {
            if (WIFEXITED(status))
                printf("Exit code: %d\n", WEXITSTATUS(status));
        }
    }
    return 0;
}
