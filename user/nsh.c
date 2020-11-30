#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAXARGS 10
#define MAXLEN 100

char whitespace[] = " \t\r\n\v";
void runcmd(char **argv, int argc);

/**
 * 输出错误信息并退出程序
 **/ 
void
panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(-1);
}

/**
 * 对fork的封装，处理异常
 **/ 
int 
fork1() {
  int pid;
  
  pid = fork();
  if (pid < 0) {
      panic("fork");
  }
  return pid;
}

/**
 * 接收输入的命令
 **/ 
int
getcmd(char * buf, int nbuf) {
  fprintf(2, "@ ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if (buf[0] == 0) { // EOF
    return -1;
  }
  return 0;
}

/**
 * 将字符串根据空格分割为数组
 * @param cmd 命令字符串
 * @param argv 参数数组
 * @return 命令个数
 **/
int
split(char * cmd, char ** argv) {
  int i = 0, j = 0, len = 0;

  len = strlen(cmd);
  while (i < len && cmd[i]) {
    while (i < len && strchr(whitespace, cmd[i])) {   // 跳过空格部分
      i++;
    }
    if (i < len) {  
      argv[j++] = cmd + i;   // 将每个参数的开始位置放入指针数组中
    }
    while (i < len && !strchr(whitespace, cmd[i])) {  // 跳过字符部分
      i++;
    }
    cmd[i++] = 0;            // 在每个参数后的第一个空格处用'\0'进行截断
  }
  argv[j] = 0;               // 表示参数数组的结束
  return j;                  // 返回参数个数
}

/**
 * 管道实现
 * @param argv 参数列表
 * @param argc 参数个数
 * @param index | 符号在数组中的索引
 **/
void
runpipe(char **argv, int argc, int index) {
  int p[2];

  pipe(p);
  if (fork1() == 0) {
    close(1);                              // 关闭标准输出
    dup(p[1]);                             // 复制管道中fd
    close(p[0]);      
    close(p[1]);
    runcmd(argv, index);                   // 递归执行左侧命令
  } 
  if (fork1() == 0) {
    close(0);
    dup(p[0]);
    close(p[0]);
    close(p[1]);
    runcmd(argv+index+1, argc-index-1);    // 递归执行右侧命令
  }
  // 关闭不需要的fd
  close(p[0]);
  close(p[1]);
  // 等待两个子进程结束
  wait(0);
  wait(0);
}

/**
 * 重定向实现
 * @param tok 重定向符号 < 或 >
 * @param file 文件路径
 **/
void
runredir(char tok, char * file) {
  switch (tok) {
  case '<':
    close(0);   
    open(file, O_RDONLY);
    break;
  case '>':
    close(1);
    open(file, O_WRONLY|O_CREATE);
    break;  
  default:
    break;
  }
}

/**
 * 解析并执行命令
 * @param argv 参数列表
 * @param argc 参数个数
 **/
void
runcmd(char **argv, int argc) {
  int i, j = 0;
  char tok;
  char *cmd[MAXARGS];

  for (i = 0; i < argc; i++) {
    if (strcmp(argv[i], "|") == 0) {
      runpipe(argv, argc, i);       // 处理pipe
      return;
    }
  }
  for (i = 0; i < argc; i++) {
    tok = argv[i][0];               // 该参数的第一个字符
    if (strchr("<>", tok)) {
      if (argv[i][1]) {
        runredir(tok, argv[i]+1);   // 处理重定向
      } else {
        if (i == argc-1) {          // 文件参数
          panic("missing file for redirection");
        }
        runredir(tok, argv[i+1]);   // 处理重定向
        i++;
      }
    } else {
      cmd[j++] = argv[i];           // 收集参数
    }
  }
  cmd[j] = 0;
  exec(cmd[0], cmd);                // 执行命令
}

int 
main(void) {
  char buf[MAXLEN];        // 用于接收命令的字符串
  char *argv[MAXARGS];     // 字符串数组
  int argc;                // 参数个数

  while (getcmd(buf, sizeof(buf)) >= 0) {
    if (fork1() == 0) {
      argc = split(buf, argv);  // 根据空格分割为字符串数组
      runcmd(argv, argc);       // 解析并执行命令
    }
    wait(0);                    // 等待子进程退出
  }
  exit(0);
}
