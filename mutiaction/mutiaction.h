
#ifndef SERVICE_MUTIACTIONSDK_H
#define SERVICE_MUTIACTIONSDK_H

#define MUTIACTION_URL  "playurl"
#define MUTIACTION_TTS  "playtts"
#define MUTIACTION_CTRL  "playctrl"

INT64 MutiAction_Get_WaitTime(void);
int MutiAction_Action(const char *msgType, void *msg);
int MutiAction_Send(char *sendbuff);
int MutiAction_Recv(char *buff, int buff_size);
int MutiAction_Init(void);
int MutiAction_Exit(void);

#endif  //!SERVICE_MUTIACTIONSDK_H

