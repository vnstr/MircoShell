//
// Created by Gueren Drive on 4/22/21.
//

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SIDE_OUT 0
#define SIDE_IN 1

#define STDIN 0
#define STDOUT 1
#define STDERR 1

#define TYPE_END 0
#define TYPE_PIPE 1
#define TYPE_BREAK 2

typedef struct s_list {
  char **args;
  int length;
  int type;
  int pipes[2];
  struct s_list *next;
  struct s_list *prev;
} t_list;

int ft_strlen(const char *s) {
  int i = 0;

  while (s[i]) {
    ++i;
  }
  return i;
}

void print_error(const char *s) {
  write(2, s, ft_strlen(s));
}

int exit_fatal() {
  print_error("error: fatal\n");
  exit(EXIT_FAILURE);
}

char *ft_strdup(const char *s) {
  char *copy = (char*)malloc(sizeof(char) * (ft_strlen(s) + 1));
  int i = 0;

  if (!copy) {
    exit_fatal();
  }
  while (s[i]) {
    copy[i] = s[i];
    ++i;
  }
  copy[i] = '\0';
  return copy;
}

void add_arg(t_list *cmd, const char *arg) {
  char **new_args = (char**)malloc(sizeof(char*) * (cmd->length + 2));
  int i = 0;

  if (!new_args) {
    exit_fatal();
  }
  while (i < cmd->length) {
    new_args[i] = cmd->args[i];
    ++i;
  }
  new_args[i++] = ft_strdup(arg);
  new_args[i] = NULL;
  if (cmd->length) {
    free(cmd->args);
  }
  cmd->args = new_args;
  cmd->length += 1;
}

void list_push_back(t_list **begin, const char *arg) {
  t_list *new = (t_list*)malloc(sizeof(t_list));
  t_list *current = *begin;

  if (!new) {
    exit_fatal();
  }
  new->args   = NULL;
  new->length = 0;
  new->type   = TYPE_END;
  new->next   = NULL;
  new->prev   = NULL;
  add_arg(new, arg);
  if (!current) {
    *begin = new;
    return;
  }
  while (current->next) {
    current = current->next;
  }
  current->next = new;
  new->prev = current;
}

void parse_arg(t_list **begin, const char *arg) {
  int is_break = (strcmp(";", arg) == 0);

  if (is_break && !*begin) {
    return;
  }
  if (is_break) {
    (*begin)->type = TYPE_BREAK;
    return;
  }
  if (strcmp("|", arg) == 0) {
    (*begin)->type = TYPE_PIPE;
    return;
  }
  if (!*begin || (*begin)->type != TYPE_END) {
    list_push_back(begin, arg);
    return;
  }
  add_arg(*begin, arg);
}

int exec_cmd(t_list *cmd, char **env) {
  pid_t pid     = 0;
  int ret       = EXIT_FAILURE;
  int status    = 0;
  int pipe_open = 0;

  if (cmd->type == TYPE_PIPE || (cmd->prev && cmd->prev->type == TYPE_PIPE)) {
    pipe_open = 1;
    if (pipe(cmd->pipes)) {
      exit_fatal();
    }
  }
  pid = fork();
  if (!pid) {
    if (cmd->type == TYPE_PIPE &&
        dup2(cmd->pipes[SIDE_IN], STDOUT) < 0) {
      exit_fatal();
    }
    if (cmd->prev &&
        cmd->prev->type == TYPE_PIPE &&
        dup2(cmd->prev->pipes[SIDE_OUT], STDIN) < 0) {
      exit_fatal();
    }
    ret = execve(cmd->args[0], cmd->args, env);
    if (ret < 0) {
      print_error("error: cannot execute ");
      print_error(cmd->args[0]);
      print_error("\n");
    }
    exit(ret);
  }
  waitpid(pid, &status, 0);
  if (pipe_open) {
    close(cmd->pipes[STDIN]);
    if (!cmd->next || cmd->type == TYPE_BREAK) {
      close(cmd->pipes[SIDE_OUT]);
    }
  }
  if (cmd->prev && cmd->prev->type == TYPE_PIPE) {
    close(cmd->prev->pipes[SIDE_OUT]);
  }
  if (WIFEXITED(status)) {
    ret = WEXITSTATUS(status);
  }
  return ret;
}

int exec_cmds(t_list **cmds, char **env) {
  t_list *current = *cmds;
  int ret = EXIT_SUCCESS;

  while (current) {
    if (strcmp("cd", current->args[0]) == 0) {
      if (current->length < 2) {
        print_error("error: cd: bad arguments\n");
        ret = EXIT_FAILURE;
      } else if (chdir(current->args[1])) {
        print_error("error: cd:: cannot change directory to ");
        print_error(current->args[1]);
        print_error("\n");
        ret = EXIT_FAILURE;
      }
    } else {
      ret = exec_cmd(current, env);
    }
    current = current->next;
  }
  return ret;
}

int main(int argc, char **argv, char **env) {
  t_list *cmds = NULL;
  int i   = 1;
  int ret = EXIT_SUCCESS;

  while (i < argc) {
    parse_arg(&cmds, argv[i]);
    ++i;
  }
  if (cmds) {
    ret = exec_cmds(&cmds, env);
  }
  return ret;
}
