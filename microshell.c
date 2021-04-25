//
// Created by Gueren Drive on 4/22/21.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SIDE_OUT 0
#define SIDE_IN 1

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define TYPE_END 0
#define TYPE_PIPE 1
#define TYPE_BREAK 2

typedef struct s_list {
  char **args;
  int  length;
  int  type;
  int  pipes[2];
  struct s_list *next;
  struct s_list *prev;
} t_list;

size_t ft_strlen(char *s) {
  size_t i = 0;

  while (s[i]) {
    ++i;
  }
  return i;
}

char *ft_strdup(char *s) {
  char *copy = (char*)malloc(sizeof(char) * (ft_strlen(s) + 1));
  size_t i = 0;

  if (!copy) {
    return NULL;
  }
  while (s[i]) {
    copy[i] = s[i];
    ++i;
  }
  copy[i] = '\0';
  return copy;
}

void list_clear(t_list **begin) {
  t_list *current = *begin;
  t_list *tmp = NULL;

  if (!current) {
    return;
  }
  while (current) {
    tmp = current;
    current = current->next;
    free(tmp);
  }
  *begin = NULL;
}

int list_add_arg(t_list *lst, char *arg) {
  char **tmp = NULL;
  int i = 0;

  tmp = (char**)malloc(sizeof(char*) * (lst->length + 2));
  if (!tmp) {
    return EXIT_FAILURE;
  }
  while (i < lst->length) {
    tmp[i] = lst->args[i];
    ++i;
  }
  if (lst->length) {
    free(lst->args);
  }
  lst->args = tmp;
  lst->args[i++] = ft_strdup(arg);
  lst->args[i] = NULL;
  lst->length += 1;
  return EXIT_SUCCESS;
}

int list_push_back(t_list **begin, char *arg) {
  t_list *new     = (t_list*)malloc(sizeof(t_list));
  t_list *current = *begin;

  if (!new) {
    return EXIT_FAILURE;
  }
  new->args   = NULL;
  new->length = 0;
  new->type   = TYPE_END;
  new->next   = NULL;
  new->prev   = NULL;
  if (!current) {
    *begin = new;
    return list_add_arg(new, arg);
  }
  while (current->next) {
    current = current->next;
  }
  current->next = new;
  new->prev     = current;
  return list_add_arg(new, arg);
}

int parse_arg(t_list **begin, char *arg) {
  int is_break = strcmp(";", arg) == 0;

  if (is_break && !*begin) {
    return EXIT_SUCCESS;
  } else if (!is_break && (!*begin || (*begin)->type > TYPE_END)) {
    return list_push_back(begin, arg);
  } else if (strcmp("|", arg) == 0) {
    (*begin)->type =TYPE_PIPE;
  } else if (is_break) {
    (*begin)->type = TYPE_BREAK;
  } else {
    return list_add_arg(*begin, arg);
  }
  return EXIT_SUCCESS;
}

int exec_cmd(t_list *cmd, char **env) {
  pid_t pid = 0;
  int ret= EXIT_FAILURE;
  int status;
  int pipe_open= 0;

  if (cmd->type == TYPE_PIPE || (cmd->prev && cmd->prev->type == TYPE_PIPE)) {
    pipe_open = 1;
    if (pipe(cmd->pipes)) {
      return EXIT_FAILURE;
    }
  }
//  pid = fork();
  if (pid < 0) {
    return EXIT_FAILURE;
  }
  if (!pid) {
    if (cmd->type == TYPE_PIPE && dup2(cmd->pipes[SIDE_IN], STDOUT) < 0) {
      return EXIT_FAILURE;
    }
    if (cmd->prev &&
        cmd->prev->type == TYPE_PIPE &&
        dup2(cmd->prev->pipes[SIDE_OUT], STDIN) < 0) {
      return EXIT_FAILURE;
    }
    ret = execve(cmd->args[0], cmd->args, env);
    if (ret < 0) {
      /*
       * ERROR
       */
    }
    exit(ret);
  }
  waitpid(pid, &status, 0);
  if (pipe_open) {
    close(cmd->pipes[SIDE_IN]);
    if (cmd->prev || cmd->type == TYPE_BREAK) {
      close(cmd->prev->pipes[SIDE_OUT]);
    }
    if (WIFEXITED(status)) {
      ret = WEXITSTATUS(status);
    }
  }
  return ret;
}

int main(int argc, char **argv, char **env) {
  t_list *cmd = NULL;
  int ret;
  int i;

  i = 1;
  while (i < argc) {
    parse_arg(&cmd, argv[i]);
    ++i;
  }
  if (cmd) {
    ret = exec_cmd(cmd, env);
  }
  list_clear(&cmd);
  return ret;
}
