#include "kernel/types.h"
#include "user/user.h"

#define NHIJOS 3
#define RONDAS 40
#define TRABAJO 20000000   // ajustar si va muy rápido o muy lento

int
main(void)
{
  for (int i = 0; i < NHIJOS; i++) {
    int pid = fork();
    if (pid == 0) {
      volatile long x = 0;
      for (int r = 0; r < RONDAS; r++) {
        for (long k = 0; k < TRABAJO; k++)
          x += k;                       // solo CPU, sin bloquearse
        printf("hijo %d (pid %d) ronda %d tick %d\n",
               i, getpid(), r, uptime());
      }
      exit(0);
    }
  }
  for (int i = 0; i < NHIJOS; i++)
    wait(0);
  exit(0);
}