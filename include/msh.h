#ifndef _my_SHELL_H
#define _my_SHELL_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <deque>
#include <string>
#include <unordered_map>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <sys/wait.h>
#include <vector>

using namespace std;

struct Variable
{
  string value;
  bool readonly;
  bool integer;

  Variable() : value(""), readonly(false), integer(false) {}

  Variable(string v, bool r = false, bool i = false)
      : value(v), readonly(r), integer(i) {}
};

class my_shell
{

public:
  deque<string> history;
  unordered_map<string, Variable> variables;
  unordered_map<std::string, std::string> aliases;
  void exec_help();
  void exec_unalias(char **argv);
  static const int history_max_size = 10;
  void parse_command(char *cmd, char **cmdTokens);
  void exec_command(char **argv);
  void exec_cd(char **argv);
  bool has_pipe(char **cmdTokens, int &pipePos);
  void exec_pipe(char **argv, int pipePos);
  void exec_exit();
  void exec_export(char **argv);
  void exec_declare(char **argv);
  void exec_unset(char **argv);
  bool isQuit(char *cmd);
  void exec_redirect(char **argv, bool append);
  void exec_history();
  void exec_alias(char **argv);
};

#endif
