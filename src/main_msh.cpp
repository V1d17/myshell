

#include <msh.h>

using namespace std;

int main() {
  char cmd[81];
  char *cmdTokens[25];
  my_shell *shell = new my_shell();
  system("clear");
  cout << "msh> ";
  while (fgets(cmd, sizeof(cmd), stdin)) {
    if (cmd[0] != '\n') {
      shell->parse_command(cmd, cmdTokens);
      if (shell->isQuit(*cmdTokens)) {
        exit(0);
      } else if (strcmp(cmdTokens[0], "clear") == 0) {
        // Clear the terminal
        system("clear");
      } else {
        shell->exec_command(cmdTokens);
      }
    }
    cout << "msh> ";
  }
  cout << endl;
  exit(0);
}
