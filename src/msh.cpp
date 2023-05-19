#include <msh.h>

using namespace std;

// Function to parse the input command
void my_shell::parse_command(char *cmd, char **cmdTokens)
{
    // Don't add 'history' command to history
    if (strncmp(cmd, "history", 7) != 0)
    {
        // If command is not 'history', add it to history
        history.push_back(cmd);
    }
    
    // If history size exceeds max size, remove oldest command
    if (history.size() > history_max_size)
    {
        history.pop_front();
    }
    
    // Tokenizing the command
    char *s = strtok(cmd, " ");
    int i = 0;
    while (s != NULL)
    {
        cmdTokens[i] = s;
        i++;
        s = strtok(NULL, " ");
    }

    // Removing the trailing newline character
    int length = strlen(cmdTokens[i - 1]);
    char *newStr = new char[length];
    memcpy(newStr, cmdTokens[i - 1], length - 1);
    newStr[length - 1] = '\0';
    cmdTokens[i - 1] = newStr;
    cmdTokens[i] = NULL;
}

// Function to execute the parsed command
void my_shell::exec_command(char **argv)
{
    int pipePos;
    string cmd = argv[0];
    
    // If the command is an alias, replace it with the actual command
    if (aliases.find(cmd) != aliases.end())
    {
        cmd = aliases[cmd];
        vector<char *> new_argv; // Using vector for easier management
        char *token = strtok(&cmd[0], " ");
        while (token != NULL)
        {
            new_argv.push_back(token);
            token = strtok(NULL, " ");
        }

        // Append the rest of the original arguments
        int i = 1;
        while (argv[i] != NULL)
        {
            new_argv.push_back(argv[i]);
            i++;
        }

        // Null-terminate the new argv array
        new_argv.push_back(NULL); 

        // Copy the new_argv back into the original argv
        for (i = 0; i < new_argv.size(); i++)
        {
            argv[i] = new_argv[i];
        }
    }

    // Check if the command includes a pipe, then execute the pipe
    if (has_pipe(argv, pipePos))
    {
        exec_pipe(argv, pipePos);
    }
    else if (strcmp(argv[0], "history") == 0) // If the command is 'history', execute history
    {
        exec_history();
    }
    else if (strcmp(argv[0], "help") == 0) // If the command is 'help', execute help
    {
        exec_help();
    }
    else if (strcmp(argv[0], "declare") == 0)// If the command is 'declare', execute declare
    {
        exec_declare(argv);
    }
    else if (strcmp(argv[0], "cd") == 0) // If the command is 'cd', execute cd
    {
        exec_cd(argv);
    }
    else if (strcmp(argv[0], "quit") == 0 || strcmp(argv[0], "exit") == 0) // If the command is 'quit' or 'exit', exit the shell
    {
        exec_exit();
    }
    else if (strcmp(argv[0], "export") == 0) // If the command is 'export', execute export
    {
        exec_export(argv);
    }
    else if (strcmp(argv[0], "unalias") == 0) // If the command is 'unalias', execute unalias
    {
        exec_unalias(argv);
    }
    else if (strcmp(argv[0], "unset") == 0) // If the command is 'unset', execute unset
    {
        exec_unset(argv);
    }
    else if (strcmp(argv[0], "alias") == 0) // If the command is 'alias', execute alias
    {
        exec_alias(argv);
    }
    else // If none of the built-in commands match, treat it as a regular system command
    {
        int i = 0;
        while (argv[i] != NULL)
        {
            // Redirect output if '>' or '>>' is detected
            if (strcmp(argv[i], ">") == 0)
            {
                exec_redirect(argv, false);
                return;
            }
            else if (strcmp(argv[i], ">>") == 0)
            {
                exec_redirect(argv, true);
                return;
            }
            i++;
        }
        // Fork and execute the command
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("msh");
            return;
        }
        else if (pid == 0)
        {
            // In child process, execute the command
            execvp(argv[0], argv);
            // If execution fails, print error and exit
            perror("msh");
            exit(EXIT_FAILURE);
        }
        else
        {
            // Parent process waits for the child to finish
            int status;
            do {
                pid_t w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
                if (w == -1) {
                    perror("msh");
                    return;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }
}

void my_shell::exec_unalias(char **argv)
{
    if (argv[1] == NULL)
    {
        fprintf(stderr, "msh: expected argument to \"unalias\"\n");
    }
    else
    {
        if (strcmp(argv[1], "-a") == 0) // If the '-a' flag is specified
        {
            aliases.clear(); // Clear all aliases
        }
        else
        {
            // argv[1] is the alias to be removed
            auto alias_itr = aliases.find(argv[1]);

            // If the alias is found, erase it
            if(alias_itr != aliases.end())
            {
                aliases.erase(alias_itr);
            }
            else
            {
                fprintf(stderr, "msh: unalias: %s: not found\n", argv[1]);
            }
        }
    }
}

bool my_shell::isQuit(char *cmd)
{
   //checks for quit command
    if (strcmp(cmd, "quit") == 0)
    {
        return true;
    }
    return false;
}

void my_shell::exec_history()
{   
    //prints history in reverse order
    int i = 1;
    for (auto it = history.rbegin(); it != history.rend(); ++it)
    {
        cout << i << ": " << *it;
        ++i;
    }
}

void my_shell::exec_declare(char **argv)
{  //prints all the declared variable if no other arguement provided 
    if (argv[1] == NULL)
    {
        for (const auto &kv : variables)
        {
            const Variable &var = kv.second;
            cout << (var.readonly ? "declare -r " : "")
                 << (var.integer ? "declare -i " : "")
                 << kv.first << "=" << var.value << endl;
        }
    }
    else
    {
        char *env_var = strtok(argv[1], "=");
        char *env_val = strtok(NULL, "=");
        if (env_var != NULL && env_val != NULL)
        {
            // Attempt to parse the value as an integer
            try
            {
                int value = stoi(env_val);
                // If successful, store it as an integer variable
                variables[env_var] = Variable(env_val, false, true);
            }
            catch (invalid_argument &e)
            {
                // If stoi throws an exception, it's not an integer, so store as a string
                variables[env_var] = Variable(env_val);
            }
        }
        else
        {
            perror("msh");
        }
    }
}

void my_shell::exec_redirect(char **argv, bool append)
{
    int i = 0;
    while (argv[i] != NULL)
    {
        if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], ">>") == 0)
        {
            char *file = argv[i + 1];
            argv[i] = NULL;
            int fd;
            if (append)
            { // '>>' operator
                fd = open(file, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
            }
            else
            { // '>' operator
                fd = open(file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
            }

            if (fd == -1)
            {
                perror("msh");
                return;
            }

            pid_t pid = fork();

            if (pid < 0)
            {
                perror("msh");
                return;
            }
            else if (pid == 0)
            {
                dup2(fd, STDOUT_FILENO);
                close(fd);
                execvp(argv[0], argv);
                perror("msh");
                exit(EXIT_FAILURE);
            }
            else
            {
                close(fd);
                waitpid(pid, NULL, 0);
            }

            return;
        }

        i++;
    }
}



void my_shell::exec_alias(char **argv)
{
    if (argv[1] == NULL)
    {
        // If no argument is given, print all aliases
        for (const auto &pair : aliases)
        {
            std::cout << pair.first << "='" << pair.second << "'\n";
        }
    }
    else
    {
        // Extract alias and command
        char *alias_name = argv[1];
        char *result = new char[200];
        strcpy(result, argv[3]);

        for (int i = 4; argv[i] != nullptr; i++)
        {

            strcat(result, " ");
            strcat(result, argv[i]);
        }

        if (alias_name != NULL)
        {

            aliases[alias_name] = result;
        }
        else
        {
            perror("msh");
        }
    }
}

void my_shell::exec_cd(char **argv)
{
    if (argv[1] == NULL)
    {
        fprintf(stderr, "msh: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(argv[1]) != 0)
        {
            perror("msh");
        }
    }
}

void my_shell::exec_pipe(char **argv, int pipePos)
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("msh");
        return;
    }

    char *leftCmd[pipePos + 1];
    memcpy(leftCmd, &argv[0], pipePos * sizeof(char *));
    leftCmd[pipePos] = NULL;

    char *rightCmd[100];
    memcpy(rightCmd, &argv[pipePos + 1], (100 - pipePos - 1) * sizeof(char *));

    pid_t leftPid = fork();

    if (leftPid < 0)
    {
        perror("msh");
        return;
    }
    else if (leftPid == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execvp(leftCmd[0], leftCmd);
        perror("msh");
        exit(EXIT_FAILURE);
    }
    else
    {
        pid_t rightPid = fork();

        if (rightPid < 0)
        {
            perror("msh");
            return;
        }
        else if (rightPid == 0)
        {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);

            execvp(rightCmd[0], rightCmd);
            perror("msh");
            exit(EXIT_FAILURE);
        }
        else
        {
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(leftPid, NULL, 0);
            waitpid(rightPid, NULL, 0);
        }
    }
}

bool my_shell::has_pipe(char **cmdTokens, int &pipePos)
{   
    if(strcmp(cmdTokens[0],"alias")==0){
        return false;
    }
    for (int i = 0; cmdTokens[i] != NULL; i++)
    {   
        if (strcmp(cmdTokens[i], "|") == 0)
        {
            pipePos = i;
            return true;
        }
    }
    return false;
}

void my_shell::exec_exit()
{
    exit(0);
}

void my_shell::exec_export(char **argv)
{
    if (argv[1] == NULL)
    {
        for (char **env = environ; *env != 0; env++)
        {
            cout << *env << endl;
        }
    }
    else
    {
        char *env_var = strtok(argv[1], "=");
        char *env_val = strtok(NULL, "=");
        if (env_var != NULL && env_val != NULL)
        {
            setenv(env_var, env_val, 1);
        }
        else
        {
            perror("msh");
        }
    }
}

void my_shell::exec_unset(char **argv)
{
    if (argv[1] == NULL)
    {
        perror("msh");
    }
    else
    {
        unsetenv(argv[1]);
    }
}

void my_shell::exec_help()
{
    int cmd_width = 30; // Set this to the length of your longest command
    cout << "List of commands supported:\n\n"
         << setw(cmd_width) << left << "cd [dir]"
         << "Changes the current directory to [dir]\n"
         << setw(cmd_width) << left << "exit"
         << "Exits the shell\n"
         << setw(cmd_width) << left << "export VAR=value"
         << "Sets the environment variable VAR to value\n"
         << setw(cmd_width) << left << "unset VAR"
         << "Unsets the environment variable VAR\n"
         << setw(cmd_width) << left << "declare VAR=value"
         << "Declares a variable\n"
         << setw(cmd_width) << left << "history"
         << "Shows the command history\n"
         << setw(cmd_width) << left << "alias name = command "
         << "Creates an alias for a command (each arg seperated by spaces)\n"
         << setw(cmd_width) << left << "unalias name"
         << "Removes an alias with name (unalias -a removes all aliases)\n"
         << setw(cmd_width) << left << "quit"
         << "Exits the shell\n"
         << setw(cmd_width) << left << "cat > [file]"
         << "Redirects output to a file (overwrites existing content)\n"
         << setw(cmd_width) << left << "cat >> [file]"
         << "Redirects output to a file (appends to existing content)\n"
         << setw(cmd_width) << left << "command1 | command2"
         << "Pipes the output of command1 to command2\n"
         << setw(cmd_width) << left << "help"
         << "Shows this help message\n"
         << "Any other command is executed normally, with arguments.\n\n";
}
