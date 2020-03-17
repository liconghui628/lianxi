#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/time.h>
#include "iotutils.h"
#include "cJSON.h"
#include "servicesdk.h"
#include "servicedata.h"
#include "cloudsdk.h"
#include "mutiaction.h"
#include "nlu.h"
#define  MODULE_TAG "Speaker-MutiActionSDK"
#include "log.h"

static char g_lastIP[16];
static UINT32 g_lastSeq = 0;
static char g_broadAddr[16];
static int g_sockfd = -1;
int g_mutiaction_isrunning = 0;
static ThreadUnit *g_mutiaction_thread = NULL;
static ThreadSignal  *gActionSig;
static int g_wait_sec = 0;
static int g_wait_usec = 0;

#define BROAD_PORT  (4004)
#define SYNC_WAIT_TIME  (3)

int MutiAction_Recv(char *recvbuff, int buff_size);
static int MutiAction_Set_WaitTime(int sec, int usec);

static int get_broadcast_addr(char *buff, int buff_size)
{
    int fd;
    int i;
    struct ifreq *ifreq;
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[512];
    log_debug0("F:%s, f:%s, L:%d\n", __FILE__, __func__, __LINE__);
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        log_error("F:%s, f:%s, L:%d, create socket error:%s\n", __FILE__, __func__, __LINE__, strerror(errno));
        return E_FAILED;
    }
    
    ifc.ifc_len = 512; 
    ifc.ifc_buf = buf;
    if (ioctl(fd, SIOCGIFCONF, (char *)&ifc) < 0){
        log_error("F:%s, f:%s, L:%d, ioctl error:%s\n", __FILE__, __func__, __LINE__, strerror(errno));
        return E_FAILED;
    }
    ifreq = (struct ifreq*)buf;
    for(i = 0; i < ifc.ifc_len/sizeof(struct ifreq); i++){
        if (strcmp(ifreq->ifr_name, "lo") != 0 && strcmp(ifreq->ifr_name, "usb0") != 0) {
            break;
        }
        ifreq ++;
    }
    if (i > ifc.ifc_len/sizeof(struct ifreq)) {
        log_error("F:%s, f:%s, L:%d, did not find valuable eth\n", __FILE__, __func__, __LINE__);
        return E_FAILED;
    }
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifreq->ifr_name);  
    if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0) {
        log_error("F:%s, f:%s, L:%d, ioctl error:%s\n", __FILE__, __func__, __LINE__, strerror(errno));
        return E_FAILED;
    }
    log_debug0("F:%s, f:%s, L:%d, broadcast addr:%s\n", __FILE__, __func__, __LINE__, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_broadaddr))->sin_addr));
    strncpy(buff, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_broadaddr))->sin_addr), buff_size);
    close(fd);
    return E_SUCCESS;
}

static int mutiaction_prepare(void)
{
    log_debug0("F:%s, f:%s, L:%d\n", __FILE__, __func__, __LINE__);
    // 创建socket
    int optval = 1;
    struct sockaddr_in client_addr;

    memset(&client_addr, 0, sizeof(client_addr));

    g_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sockfd < 0) {
        log_error("F:%s, f:%s, L:%d create socket failed!\n", __FILE__, __func__, __LINE__);
        return E_FAILED ;
    }
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port = htons(BROAD_PORT);
    if (bind(g_sockfd, (struct sockaddr *)&client_addr, sizeof(struct sockaddr)) < 0){
        log_error("F:%s, f:%s, L:%d bind failed!\n", __FILE__, __func__, __LINE__);
        goto failed ;
    }
   
    if (setsockopt(g_sockfd, SOL_SOCKET, SO_BROADCAST|SO_REUSEADDR, &optval, sizeof(int)) < 0){
        log_error("F:%s, f:%s, L:%d setsockopt failed!\n", __FILE__, __func__, __LINE__);
        goto failed ;
    }

    memset(g_broadAddr, 0, sizeof(g_broadAddr));
    if (get_broadcast_addr(g_broadAddr, sizeof(g_broadAddr)) != E_SUCCESS) {
        goto failed;
    }
    log_debug0("F:%s, f:%s, L:%d, g_sockfd=%d, g_broadAddr=%s\n", __FILE__, __func__, __LINE__, g_sockfd, g_broadAddr);
    return E_SUCCESS;

failed:
    close(g_sockfd);
    g_sockfd = -1;
    return E_FAILED;
}

static void mutiaction_thread(void *data)
{
    log_debug0("F:%s, f:%s, L:%d\n", __FILE__, __func__, __LINE__);
    if (mutiaction_prepare() != E_SUCCESS) {
        log_error("F:%s, f:%s, L:%d mutiaction_prepare failed, thread exit\n", __FILE__, __func__, __LINE__);
        return ;
    }
    char recvbuff[1024];
    while (g_mutiaction_isrunning) {
        memset(recvbuff, 0, sizeof(recvbuff));
        MutiAction_Recv(recvbuff, sizeof(recvbuff));
    }

    log_error("F:%s, f:%s, L:%d  thread exit\n", __FILE__, __func__, __LINE__);
}

static int commond_send_request(char *msgType, void *msg)
{
    int ret = E_FAILED;
    cJSON *data = NULL, *param = NULL;
    CommandRequest *cmdreq;
	log_debug0("F:%s, f:%s, L:%d, \n", __FILE__, __func__, __LINE__);

    cmdreq = CommandRequest_New();
    if (cmdreq == NULL) {
        log_error("F:%s, f:%s, L:%d, req new failed !\n", __FILE__, __func__, __LINE__);
        return ret;
    }

    data = cJSON_CreateObject();
    if(data == NULL){
        log_error("F:%s, f:%s, L:%d cJSON create(data) failed\n", __FILE__, __func__, __LINE__);
        CommandRequest_Free(cmdreq);
        return ret;
    }
    cmdreq->data = data;
	//cmdreq->source_serviceID = SERVICEID_CLIENT;
	cmdreq->source_serviceID = SERVICEID_ACTION;
    if (strcmp(msgType, MUTIACTION_URL) == 0) {
	    cmdreq->target_serviceID = SERVICEID_ACTION;
        cJSON_AddStringToObject(data, "action", "playurl");
        cJSON_AddStringToObject(data, "param", (char*)msg);
    } else if (strcmp(msgType, MUTIACTION_TTS) == 0){
	    cmdreq->target_serviceID = SERVICEID_ACTION;
        cJSON_AddStringToObject(data, "action", "playtts");
        cJSON_AddStringToObject(data, "param", (char*)msg);
    } else if (strcmp(msgType, MUTIACTION_CTRL) == 0){
	    cmdreq->target_serviceID = SERVICEID_CTRL;
        param = cJSON_CreateObject();
        if (param == NULL) {
            CommandRequest_Free(cmdreq);
            return ret;
        }
        if (strcmp((char*)msg, "play") == 0) {
            //cJSON_AddStringToObject(param, NLU_CTRL_OPERATION, NLU_CTRL_OPERATION_PLAY);
            cJSON_AddNumberToObject(data, NLU_MUSIC_CLIENT_HANDLE, 1);
        } else if (strcmp((char*)msg, "pause") == 0 || strcmp((char*)msg, "stop") == 0){
            //cJSON_AddStringToObject(param, NLU_CTRL_OPERATION, NLU_CTRL_OPERATION_PAUSE);
            cJSON_AddNumberToObject(data, NLU_MUSIC_CLIENT_HANDLE, 2);
        } else if (strcmp((char*)msg, "next") == 0){
            //cJSON_AddStringToObject(param, NLU_CTRL_OPERATION, NLU_CTRL_OPERATION_NEXT);
            cJSON_AddNumberToObject(data, NLU_MUSIC_CLIENT_PRENEXT, 1);
        } else if (strcmp((char*)msg, "prev") == 0){
            //cJSON_AddStringToObject(param, NLU_CTRL_OPERATION, NLU_CTRL_OPERATION_PREVIOUS);
            cJSON_AddNumberToObject(data, NLU_MUSIC_CLIENT_PRENEXT, 0);
        }
        cJSON_AddItemToObject(data, "param", param);
    } else {
        log_error("F:%s, f:%s, L:%d unsupport msgType:%s\n", __FILE__, __func__, __LINE__, msgType);
        CommandRequest_Free(cmdreq);
        return ret;
    }
    return CommandRequest_Send(cmdreq);
}

static int MutiAction_Recv_Handle(char *recvbuff, char *fromip) 
{
    log_debug0("F:%s, f:%s, L:%d\n", __FILE__, __func__, __LINE__);
    int ret = E_FAILED;
    cJSON *data = NULL, *msgType = NULL;
    cJSON *userID = NULL;
    cJSON *gardgetTypeID = NULL;
    cJSON *message = NULL;
    cJSON *sec = NULL;
    cJSON *usec = NULL;
    cJSON *sequenceID = NULL;
    struct timeval tv;

    if (recvbuff == NULL) {
        log_error("F:%s, f:%s, L:%d param error\n", __FILE__, __func__, __LINE__);
        goto failed;
    }

    data = cJSON_Parse(recvbuff);
    if (data){
        // 判断是否是重复的网络包
        log_debug0("F:%s, f:%s, L:%d, g_lastIP=%s, g_lastSeq=%d\n", __FILE__, __func__, __LINE__, g_lastIP, g_lastSeq);
        sequenceID = cJSON_GetObjectItem(data, "sequenceID");
        if (sequenceID) {
            if (strcmp(g_lastIP, fromip) == 0 && g_lastSeq == sequenceID->valueint) {
                log_debug0("F:%s, f:%s, L:%d, repeat msg\n", __FILE__, __func__, __LINE__);
                goto failed;
            }
        }
        iots_strcpys(g_lastIP, sizeof(g_lastIP), fromip);
        g_lastSeq = sequenceID->valueint;

        // 判断是否来自同一用户
        userID = cJSON_GetObjectItem(data, "userID");
        if (userID && userID->valuestring) {
            if (strcmp(userID->valuestring, IOTDM_GetUserID()) != 0) {
                log_debug0("F:%s, f:%s, L:%d, from another user, skip\n", __FILE__, __func__, __LINE__);
                goto failed;
            }
        }
        // 判断是否是相同类型的设备
        gardgetTypeID = cJSON_GetObjectItem(data, "gardgetTypeID");
        if (gardgetTypeID && gardgetTypeID->valueint != SPEAKER_GARDGET_TYPE_ID){
            log_debug0("F:%s, f:%s, L:%d, different gardget type id, skip\n", __FILE__, __func__, __LINE__);
            goto failed;
        }
        msgType = cJSON_GetObjectItem(data, "msgType");
        message = cJSON_GetObjectItem(data, "message");
        if (msgType && msgType->valuestring && message && message->valuestring) {
            sec = cJSON_GetObjectItem(data, "sec");
            usec = cJSON_GetObjectItem(data, "usec");
            if (sec && usec) {
                gettimeofday(&tv, NULL);
                if (strcmp(msgType->valuestring, MUTIACTION_URL) == 0) {
                    log_debug0("F:%s, f:%s, L:%d, should play at %dsec %dusec\n", __FILE__, __func__, __LINE__, sec->valueint, usec->valueint);
                    MutiAction_Set_WaitTime(sec->valueint, usec->valueint);
                } else {
                    INT64 wait_usec = (sec->valueint - tv.tv_sec) * 1000000 + (usec->valueint - tv.tv_usec);
                    log_debug0("F:%s, f:%s, L:%d, usleep %d ms, start\n", __FILE__, __func__, __LINE__, wait_usec);
                    usleep(wait_usec);
                    log_debug0("F:%s, f:%s, L:%d, usleep %d ms, end\n", __FILE__, __func__, __LINE__, wait_usec);
                }
            }
            ret = commond_send_request(msgType->valuestring, message->valuestring);
        }
    }
failed:
    if (data)
        cJSON_Delete(data);
    return ret;
}

static int MutiAction_Set_WaitTime(int sec, int usec) 
{
    log_debug0("F:%s, f:%s, L:%d set wait time %dsec %dusec\n", __FILE__, __func__, __LINE__, sec, usec);
    g_wait_sec = sec;
    g_wait_usec = usec;
    return E_SUCCESS;
}

INT64 MutiAction_Get_WaitTime(void) 
{
    struct timeval now;
    log_debug0("F:%s, f:%s, L:%d get wait time sec=%d usec=%d\n", __FILE__, __func__, __LINE__, g_wait_sec, g_wait_usec);
    gettimeofday(&now, NULL);
    INT64 wait_time =  (g_wait_sec - now.tv_sec) * 1000000 + (g_wait_usec - now.tv_usec);
    if (wait_time < 0)
        return 0;
    return wait_time;
}

int MutiAction_Send(char *sendbuff)
{
    struct sockaddr_in theirAddr;
    int errCnt = 0;
    int i = 0;
    struct timeval now;
    cJSON *data = NULL, *msgType = NULL;

    if (sendbuff == NULL) {
        log_error("F:%s, f:%s, L:%d param error\n", __FILE__, __func__, __LINE__);
        return E_FAILED;
    }
    if (g_sockfd < 0) {
        log_error("F:%s, f:%s, L:%d g_sockfd=%d\n", __FILE__, __func__, __LINE__, g_sockfd);
        return E_FAILED;
    }
    log_debug0("F:%s, f:%s, L:%d, send: %s\n", __FILE__, __func__, __LINE__, sendbuff);

    memset(&theirAddr, 0, sizeof(struct sockaddr_in));  
    theirAddr.sin_family = AF_INET;  
    theirAddr.sin_addr.s_addr = inet_addr(g_broadAddr);  
    theirAddr.sin_port = htons(BROAD_PORT); 
    // 由于广播包接收不稳定，所以连发三次
    for (i = 0; i < 3; i++){
        if(sendto(g_sockfd, sendbuff, strlen(sendbuff), 0, (struct sockaddr *)&theirAddr, sizeof(struct sockaddr)) < 0){  
            log_debug0("F:%s, f:%s, L:%d the %d time, sendto failed\n", __FILE__, __func__, __LINE__);
            errCnt ++;
        }
    }
    if (errCnt == 3) {
        return E_FAILED;
    }

    data = cJSON_Parse(sendbuff);
    if (data){
        msgType = cJSON_GetObjectItem(data, "msgType"); 
        if (msgType && msgType->valuestring){
            if (strcmp(msgType->valuestring, MUTIACTION_URL) == 0) {
                gettimeofday(&now, NULL);
                MutiAction_Set_WaitTime(now.tv_sec + SYNC_WAIT_TIME , now.tv_usec);
            } else {
                log_debug0("F:%s, f:%s, L:%d, start usleep 500000\n", __FILE__, __func__, __LINE__);
                usleep(500000);
                log_debug0("F:%s, f:%s, L:%d, end usleep 500000\n", __FILE__, __func__, __LINE__);
            }
        }
        cJSON_Delete(data);
    }
    return E_SUCCESS;
}

int MutiAction_Recv(char *recvbuff, int buff_size)
{
    struct timeval now;
    char fromip[16];
    struct sockaddr_in recvAddr; 
    int addrLen = sizeof(struct sockaddr_in);

    log_debug0("F:%s, f:%s, L:%d\n", __FILE__, __func__, __LINE__);
    if (g_sockfd < 0) {
        log_error("F:%s, f:%s, L:%d g_sockfd=%d\n", __FILE__, __func__, __LINE__, g_sockfd);
        return E_FAILED;
    }
    if (recvbuff == NULL || buff_size <= 0) {
        log_error("F:%s, f:%s, L:%d param error\n", __FILE__, __func__, __LINE__);
        return E_FAILED;
    }

    memset(&recvAddr, 0, sizeof(struct sockaddr_in)); 
    if (recvfrom(g_sockfd, recvbuff, buff_size, 0, (struct sockaddr *)&recvAddr, (socklen_t*)&addrLen) < 0) {
        log_error("F:%s, f:%s, L:%d recvfrom failed\n", __FILE__, __func__, __LINE__);
        return E_FAILED;
    }
    iots_strcpys(fromip, sizeof(fromip), inet_ntoa(recvAddr.sin_addr));
    gettimeofday(&now, NULL);
    log_debug0("F:%s, f:%s, L:%d,[%d.%d] recv from %s, msg=%s\n", 
            __FILE__, __func__, __LINE__, now.tv_sec, now.tv_usec/1000, fromip, recvbuff);

    //不处理自己广播的指令
    if (strcmp(fromip, IOTN_LocalIP()) == 0){
        log_debug0("F:%s, f:%s, L:%d, from myself broadcast, skip\n", __FILE__, __func__, __LINE__);
        return E_SUCCESS;
    }
    return MutiAction_Recv_Handle(recvbuff, fromip);
}

int MutiAction_Action(const char *msgType, void *msg) 
{
    int ret = E_FAILED;
    char *data_str = NULL;
    cJSON *data = NULL;
    struct timeval tv;
    static int sequence = 0;


    //判断app是否打开了组网播放


    if (msgType == NULL || msg == NULL) {
        log_error("F:%s, f:%s, L:%d, param error\n", __FILE__, __func__, __LINE__);
        return ret;
    }
    log_debug0("F:%s, f:%s, L:%d, msgType=%s\n", __FILE__, __func__, __LINE__,msgType);

    data = cJSON_CreateObject();
    if (data == NULL){
        log_error("F:%s, f:%s, L:%d, cJSON_CreateObject failed\n", __FILE__, __func__, __LINE__);
        goto failed;
    }
    cJSON_AddNumberToObject(data, "sequenceID", ++sequence);
    cJSON_AddStringToObject(data, "userID", IOTDM_GetUserID());
    cJSON_AddNumberToObject(data, "gardgetTypeID", SPEAKER_GARDGET_TYPE_ID);
    gettimeofday(&tv, NULL);
    if (strcmp(msgType, MUTIACTION_URL) == 0) {
        tv.tv_sec += SYNC_WAIT_TIME;
        
        cJSON_AddStringToObject(data, "msgType", MUTIACTION_URL);
        cJSON_AddStringToObject(data, "message",(char*)msg);
        cJSON_AddNumberToObject(data, "sec", tv.tv_sec);
        cJSON_AddNumberToObject(data, "usec", tv.tv_usec);
    } else if (strcmp(msgType, MUTIACTION_TTS) == 0){
        if (tv.tv_usec + 500000 >= 1000000) {
            tv.tv_usec -= 500000;
            tv.tv_sec += 1;
        } else {
            tv.tv_usec += 500000;
        }
        cJSON_AddStringToObject(data, "msgType", MUTIACTION_TTS);
        cJSON_AddStringToObject(data, "message",(char*)msg);
        cJSON_AddNumberToObject(data, "sec", tv.tv_sec);
        cJSON_AddNumberToObject(data, "usec", tv.tv_usec);
    } else if (strcmp(msgType, MUTIACTION_CTRL) == 0){
        if (tv.tv_usec + 500000 >= 1000000) {
            tv.tv_usec -= 500000;
            tv.tv_sec += 1;
        } else {
            tv.tv_usec += 500000;
        }
        cJSON_AddStringToObject(data, "msgType", MUTIACTION_CTRL);
        cJSON_AddStringToObject(data, "message",(char*)msg);
        cJSON_AddNumberToObject(data, "sec", tv.tv_sec);
        cJSON_AddNumberToObject(data, "usec", tv.tv_usec);
    } else {
        log_error("F:%s, f:%s, L:%d, unsupport msgType:%s\n", __FILE__, __func__, __LINE__, msgType);
        goto failed;
    }

    data_str = cJSON_Print(data); 
    if (data_str){
       ret = MutiAction_Send(data_str);
    }
failed:
    if (data)
        cJSON_Delete(data);
    if (data_str)
        cJSON_MFree(data_str);
    return ret;
}

int MutiAction_Init(void)
{
    log_debug0("F:%s, f:%s, L:%d\n", __FILE__, __func__, __LINE__);
    g_mutiaction_isrunning = 1;
    g_mutiaction_thread = IOTThread_Create(MODULE_TAG, 1, mutiaction_thread, NULL);
	if (g_mutiaction_thread == NULL) {
		log_error("create thread failed!\n");
		return E_FAILED;
	}
    gActionSig = IOTThreadSignal_Create();
    if (gActionSig == NULL){
		log_error("create sig failed!\n");
		return E_FAILED;
    }
    log_debug0("F:%s, f:%s, L:%d, success\n", __FILE__, __func__, __LINE__);
    return E_SUCCESS;
}

int MutiAction_Exit(void)
{
    log_debug0("F:%s, f:%s, L:%d\n", __FILE__, __func__, __LINE__);
    g_mutiaction_isrunning = 0;
    if (g_sockfd > 0) {
        close(g_sockfd);
        g_sockfd = 1;
    }
    if (g_mutiaction_thread != NULL) {
        IOTThread_Destroy(g_mutiaction_thread);
        g_mutiaction_thread = NULL;
    }
    if(gActionSig != NULL){
        IOTThreadSignal_Destroy(gActionSig);
        gActionSig = NULL;
    }
    return E_SUCCESS;
}
