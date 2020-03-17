#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void handler(int sig) {
  char **trace = NULL;
  void *array[10];
  size_t size;
  int i = 0;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  printf("signal=%d:\n", sig);
  printf("size=%d\n", size);
  //backtrace_symbols_fd(array, size, STDERR_FILENO);
  trace = backtrace_symbols(array, size);
  for (i = 0; i < size; i ++)
  {
    printf("%s\n", trace[i]);
  }
  exit(1);
}

void baz() {
 int *foo = (int*)-1; // make a bad pointer
  printf("%d\n", *foo);       // causes segfault
}

void bar() { baz(); }
void foo() { bar(); }


int main(int argc, char **argv) {
  signal(SIGSEGV, handler);   // install our handler
  foo(); // this will call foo, bar, and baz.  baz segfaults.
}
