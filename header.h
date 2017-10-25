#ifndef HEADER
#define HEADER
enum state { idle, want_in, in_cs};

typedef struct data_struct {
  int sharedNum;
  int turn;
  int numberProc;
  enum state flag[];
} data;

#endif
