#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "service.h"
#include "cJSON.h"
#include "service.h"
#include "serviceprisdk.h"
#include "nlu.h"
#include "log.h"
#include "fileio.h"
#include "iotutils.h"
#include "iottimer.h"
#include "speakerhw.h"
#include "device_dependent.h"

#define MODULE_TAG  "ServiceDeviceDependent"
#define SECONDS_IN_ONEDAY      (24*60*60)
#define VOLUME_VALUE_MIN      (0)
#define VOLUME_VALUE_MAX      (100)

static ServiceInfo g_device_serviceinfo;
static time_t g_poweroff_confirm = 0;
static time_t g_reboot_confirm   = 0;
//定时关机中相关的定时器ID
static int g_shutdown_timerID = 0;
//定时关机中关机时间点
static int g_shutdown_time = 0;
//定时停止定时器ID
static int g_stop_timerID = 0;
//定时停止时间点
static int g_stop_time = 0;
//如果定时任务时间点发生在过去，是否现在执行
static int g_doItNow = 0;
//定时事件类型
static TimerEvent g_timer_evtType = 0;
//tts回调操作 静音/音量为0
static CBCtrl g_CBCtrl = 0;
//NLU不支持领域的默认回复
static char g_default_tts[][LONGSTRING] = {
    "现在我还不会正在学习中",
    "这个我正在学习很快就会拉",
    "这个需要研究研究我每隔一段时间都会学会些新的东西",
};

/* *
 * 出错处理
 *@param[in]  req  命令指针
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_req_handle_failedrequest(CommandRequest *req)
{
    char *str_failed = NULL;
    static int index = 0;
    log_assert(req->req_errno != E_SUCCESS);
    log_debug0("F:%s, f:%s, L:%d, handle error request (%d/%p)\n", __FILE__, __func__, __LINE__, req->req_errno, req->data);

    if (req->req_errno == E_UNSUPPORT) {
        if (++index >= sizeof(g_default_tts)/sizeof(g_default_tts[0]))
                index = 0;
        str_failed  = g_default_tts[index];
    } else {
        switch(req->req_errno) {
            case E_INVALID:
                str_failed  = "语音请求失败 数据解析错误";
                break;
            case E_NO_DATA:
                str_failed  = "未接收到语音输入";
                break;
            case E_NO_MEMORY:
                str_failed  = "语音请求失败 内存不足";
                break;
            case E_FAILED:
                str_failed  = "语音请求失败";
                break;
            default:
                str_failed  = "语音请求失败2";
                break;
        }
    }
    log_assert(str_failed != NULL);
    log_debug0("F:%s, f:%s, L:%d, [TIMEKEY] start playTTS:%s\n", __FILE__, __func__, __LINE__, str_failed);
    CommandResponse_PlayTTS(req, str_failed);

    if(req->data != NULL) {
        cJSON_DeleteItemFromObject(req->data, NLU_REPLY);
        cJSON_AddStringToObject(req->data, NLU_REPLY, str_failed);
    }
    return E_SUCCESS;
}

/* *
 * 关机/重启 确认
 *@param[in]  req  命令指针
 *@param[in]  poweroff0reboot1 关机/重启
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_req_handle_shutdown_confirm(CommandRequest *req, int poweroff0reboot1)
{
    time_t curtime = time(NULL);
    time_t *confirm;
    cJSON *param, *operation;
    char *forlog = (poweroff0reboot1==0)?"poweroff":"reboot";

    log_assert(req != NULL);

    confirm = (poweroff0reboot1==0)?(&g_poweroff_confirm):(&g_reboot_confirm);
    log_assert(*confirm != 0);


    if(*confirm <= curtime) {
        log_debug0("F:%s, f:%s, L:%d, confirm %s receive(%d/%d), ignore %s\n",
                __FILE__, __func__, __LINE__, forlog, *confirm, curtime, forlog);
        return E_FAILED;
    }

    param = cJSON_GetObjectItem(req->data, NLU_PARAM);
    if(param == NULL) {
        log_error("F:%s, f:%s, L:%d get item param failed\n", __FILE__, __func__, __LINE__);
        return E_FAILED;
    }
    operation = cJSON_GetObjectItem(param, NLU_DISPATCH_OPERATION);
    if (operation == NULL || operation->valuestring == NULL) {
        log_error("F:%s, f:%s, L:%d get item operation\n", __FILE__, __func__, __LINE__);
        return E_FAILED;
    }

    if(strcmp(operation->valuestring, "确认") == 0 || strcmp(operation->valuestring, "确定") == 0) {
        log_debug0("F:%s, f:%s, L:%d, confirm %s receive(%d/%d), start %s\n",
                __FILE__, __func__, __LINE__, forlog, *confirm, curtime, forlog);
        cJSON_DeleteItemFromObject(req->data, NLU_REPLY);
        cJSON_AddStringToObject(req->data, NLU_REPLY, "正在关机");

        if (poweroff0reboot1 == 0) {
            if (HARDWARE_SUPPORT_POWEROFF)
                Service_Shutdown(0, 0);
            else
                Service_ServiceStop_ForDevice();
        } else {
            Service_Shutdown(poweroff0reboot1, 0);
        }
    } else {
        log_debug0("F:%s, f:%s, L:%d, confirm %s receive(%d/%d), cancel %s for receive %s(0x%x)\n",
                __FILE__, __func__, __LINE__, forlog, *confirm, curtime, forlog, operation->valuestring, req->target_serviceID);
        if(req->target_serviceID == SERVICEID_DEIVCEDEP) {
            return E_FAILED;
        }
    }
    return E_SUCCESS;
}

/* *
 * 定时关机定时器的回调接口
 *@param[in]  arg  目前没用
 *
 * */
static void timer_event_cb(void *arg)
{
    if (arg == NULL) {
        log_warning("F:%s, f:%s, L:%d, arg=NULL\n", __FILE__, __func__, __LINE__);
        return ;
    }
    TimerEvent event = *((TimerEvent *)arg);
    log_debug0("F:%s, f:%s, L:%d, event=%d\n", __FILE__, __func__, __LINE__, event);
    if (event == ShutdownEvt) {
        g_shutdown_timerID = 0;
        g_shutdown_time = 0;
        if (HARDWARE_SUPPORT_POWEROFF)
            Service_Shutdown(0, 1);
        else
            Service_ServiceStop_ForDevice();
    } else if (event == StopEvt) {
        g_stop_timerID = 0;
        g_stop_time = 0;
        Service_ServiceStop_ForDevice();
    } else {
        log_warning("F:%s, f:%s, L:%d, unsupport event:%d\n", __FILE__, __func__, __LINE__, event);
    }
}

/* *
 * 把一个struct tm时间点转换成一个用语言描述的字符串
 *@param[in]  targetTM  目标时间
 *@param[in]  buff      字符串存储区
 *@param[in]  buff_len  存储区大小
 *@return     E_SUCCESS 成功
 *            E_FAILED  失败
 *
 * */
static int timekey_to_str(struct tm *targetTM, char *buff, UINT32 buff_len)
{
    struct tm curTM;
    int i = 0, hour = 0;
    char *p = NULL, *pEnd = NULL;
    time_t targetTime = 0, curTime = 0;
    INT32 diffTime = 0, passedTime = 0, remainTime = 0;

    if (targetTM == NULL || buff == NULL || buff_len <= 0) {
        log_error("F:%s, f:%s, L:%d, param error\n", __FILE__, __func__, __LINE__);
        return E_FAILED;
    }

    p = buff;
    pEnd = (buff + buff_len);
    memset(&curTM, 0, sizeof(curTM));

    targetTime = mktime(targetTM);
    curTime = time(NULL);
    localtime_r(&curTime, &curTM);
    diffTime = targetTime - curTime;
    passedTime = curTM.tm_hour*60*60 + curTM.tm_min*60 + curTM.tm_sec;
    remainTime = SECONDS_IN_ONEDAY - (curTM.tm_hour*60*60 + curTM.tm_min*60 + curTM.tm_sec);
    log_debug0("F:%s, f:%s, L:%d currentTime:%d, targetTime:%d, diffTime:%d, today passedTime:%d, today remainTime:%d\n",
            __FILE__, __func__, __LINE__, curTime, targetTime, diffTime, passedTime, remainTime);
    if (diffTime < -60) { //定时任务时间在一分钟之前则返回错误
        log_error("F:%s, f:%s, L:%d, diffTime = %d\n", __FILE__, __func__, __LINE__, diffTime);
        return E_FAILED;
    } else if (diffTime <= 0) { //定时任务时间发生在过去，并且在过去一分钟之内，则现在执行
        log_warning("F:%s, f:%s, L:%d, diffTime = %d, do now !\n", __FILE__, __func__, __LINE__, diffTime);
        g_doItNow = 1;
        return E_SUCCESS;
    } else if (diffTime < remainTime) {
        iots_strcats(p, pEnd - p, "今天");
        p = buff + iots_strlen(buff);
    } else if (diffTime < remainTime + SECONDS_IN_ONEDAY) {
        iots_strcats(p, pEnd - p, "明天");
        p = buff + iots_strlen(buff);
    } else if (diffTime < remainTime + SECONDS_IN_ONEDAY * 2) {
        iots_strcats(p, pEnd - p, "后天");
        p = buff + iots_strlen(buff);
    } else {
        log_debug0("F:%s, f:%s, L:%d targetTm->year:%d, curTm->year:%d\n",
                __FILE__, __func__, __LINE__, targetTM->tm_year + 1900, curTM.tm_year + 1900);
        if (targetTM->tm_year == curTM.tm_year) {
            i = iots_sprintfs(p, pEnd - p, "%d月%d日", targetTM->tm_mon + 1, targetTM->tm_mday);
            p += i;
        } else {
            i = iots_sprintfs(p, pEnd - p, "%d年%d月%d日", targetTM->tm_year + 1900, targetTM->tm_mon + 1, targetTM->tm_mday);
            p += i;
        }
    }

    switch (targetTM->tm_hour) {
        case 0: case 1: case 2: case 3: case 4: case 5:
            iots_strcats(p, pEnd - p, "凌晨"); break;
        case 6: case 7:
            iots_strcats(p, pEnd - p, "早上"); break;
        case 8: case 9: case 10:
            iots_strcats(p, pEnd - p, "上午"); break;
        case 11: case 12: case 13:
            iots_strcats(p, pEnd - p, "中午"); break;
        case 14: case 15: case 16: case 17: case 18:
            iots_strcats(p, pEnd - p, "下午"); break;
        case 19: case 20: case 21: case 22: case 23: case 24:
            iots_strcats(p, pEnd - p, "晚上"); break;
        default: break;
    }
    p = buff + iots_strlen(buff);
    hour = targetTM->tm_hour <= 12 ? targetTM->tm_hour : targetTM->tm_hour - 12;

    if (targetTM->tm_min == 0) {
        i = iots_sprintfs(p, pEnd - p, "%d点", hour);
        p = buff + i;
    } else if (targetTM->tm_min == 30) {
        i = iots_sprintfs(p, pEnd - p, "%d点半", hour);
        p = buff + i;
    } else {
        i = iots_sprintfs(p, pEnd - p, "%d点%d分", hour, targetTM->tm_min);
        p = buff + i;
    }
    log_debug0("F:%s, f:%s, L:%d len=%d,buff=%s\n", __FILE__, __func__, __LINE__, pEnd - buff, buff);

    return E_SUCCESS;
}

/* *
 * 调节音量
 * @param[in]  v  音量值
 *
 * */
static void vol_adjust_ctrl(UINT32 v)
{
    SpeakerData *speaker_data = Speaker_GetData();
    log_assert(speaker_data != NULL);
    log_debug0("F:%s, f:%s, L:%d, set volume(%u->%u)\n",
            __FILE__, __func__, __LINE__,speaker_data->main_volume, v);
    if(v != speaker_data->main_volume) {
        speaker_data->main_volume = v;
        Speaker_SetVolume(speaker_data->main_volume, 0);
        Service_CallEventCB(SERVICE_EVENT_VOLUME_CHANGED, &speaker_data->main_volume);
    }
}

/* *
 * 静音&&取消静音
 * @param[in]  isMute  1-静音，0-取消静音
 *
 * */
static void vol_mute_ctrl(INT32 isMute)
{
    SpeakerData *speaker_data = Speaker_GetData();
    log_assert(speaker_data != NULL);
    log_debug0("F:%s, f:%s, L:%d set volume mute(%d->%d)\n",
            __FILE__, __func__, __LINE__,speaker_data->is_volmute, isMute);
    if (isMute != speaker_data->is_volmute) {
        speaker_data->is_volmute = isMute;
        Speaker_SetVolumeMute(speaker_data->is_volmute);
        Service_CallEventCB(SERVICE_EVENT_VOLMUTE_CHANGED, &speaker_data->is_volmute);
    }
}

/* *
 * TTS播放完成回调，静音
 *
 * */
static void play_tts_endcb(void *data)
{
    if (data == NULL) {
        log_error("F:%s, f:%s, L:%d data=NULL \n", __FILE__, __func__, __LINE__);
        return ;
    }
    CBCtrl *cbCtrl = (CBCtrl*)data;
    log_debug0("F:%s, f:%s, L:%d cbCtrl=%d \n", __FILE__, __func__, __LINE__, *cbCtrl);
    if (*cbCtrl == SetMute) {
        //静音
        vol_mute_ctrl(1);
    } else if (*cbCtrl == SetZero) {
        //音量设为0
        vol_adjust_ctrl(0);
    }
}

/* *
 * 定时关机
 *@param[in]  datestr 日期
 *@param[in]  timestr 时间
 *@param[in]  tts      TTS存储区
 *@param[in]  tts_len  TTS存储区大小
 *@return     E_SUCCESS 成功
 *            E_FAILED  失败
 *
 * */
static int device_timed_event_handle(TimerEvent eventType, const char *datestr, const char *timestr, char *tts, UINT32 tts_len)
{
    iot_time timekey;
    char time_str[128];
    struct tm targetTM;
    INT32 targetTime = 0, diffTime = 0;
    //datestr 可以为NULL, 为NULL时表示今天
    if (timestr == NULL || tts == NULL || tts_len <= 0) {
        log_error("F:%s, f:%s, L:%d, param error\n", __FILE__, __func__, __LINE__);
        return E_FAILED;
    }

    memset(&timekey, 0, sizeof(timekey));
    memset(&targetTM, 0, sizeof(targetTM));
    memset(time_str, 0, sizeof(time_str));

    if (iots_sscanf(timestr, "%02d:%02d:%02d", &timekey.hour, &timekey.minute, &timekey.second) == 3) {
        targetTM.tm_hour = timekey.hour;
        targetTM.tm_min = timekey.minute;
        targetTM.tm_sec = timekey.second;
        //datestr 为NULL时，则日期为今天
        if (datestr == NULL) {
            time_t timep = time(NULL);
            struct tm tm;
            localtime_r(&timep, &tm);
            targetTM.tm_year = tm.tm_year;
            targetTM.tm_mon  = tm.tm_mon;
            targetTM.tm_mday = tm.tm_mday;
        } else {
            if (iots_sscanf(datestr, "%04d%02d%02d", &timekey.year, &timekey.month, &timekey.day) == 3) {
                targetTM.tm_year = timekey.year - 1900;
                targetTM.tm_mon = timekey.month - 1;
                targetTM.tm_mday = timekey.day;
            } else {
                log_error("F:%s, f:%s, L:%d, unsupported dateMode:%s\n", __FILE__, __func__, __LINE__, datestr);
                return E_FAILED;
            }
        }
        log_debug0("F:%s, f:%s, L:%d, %04d/%02d/%02d %02d:%02d:%02d\n",
                __FILE__, __func__, __LINE__, targetTM.tm_year+1900, targetTM.tm_mon+1, targetTM.tm_mday, targetTM.tm_hour, targetTM.tm_min, targetTM.tm_sec);
        //把时间转换为描述性字符串
        if ( timekey_to_str(&targetTM, time_str, sizeof(time_str)) == E_FAILED)
            return E_FAILED;
        else if (g_doItNow == 1) //现在执行
            return E_SUCCESS;
        else {
            targetTime =  mktime(&targetTM);
            g_timer_evtType = eventType;
            if (eventType == ShutdownEvt && g_shutdown_time != targetTime) {
            //定时关机
                g_shutdown_time = targetTime;
                diffTime = g_shutdown_time - time(NULL);
                log_debug0("F:%s, f:%s, L:%d, old shutdown timerID:%d\n", __FILE__, __func__, __LINE__, g_shutdown_timerID);
                if(g_shutdown_timerID != 0)
                    IOTTMR_delTimer(g_shutdown_timerID);
                g_shutdown_timerID = IOTTMR_addTimer(timer_event_cb, &g_timer_evtType, diffTime, 1);
                log_debug0("F:%s, f:%s, L:%d, new shutdown timerID:%d\n", __FILE__, __func__, __LINE__, g_shutdown_timerID);
            } else if (eventType == StopEvt && g_stop_time != targetTime) {
            //定时停止
                g_stop_time = targetTime;
                diffTime = g_stop_time - time(NULL);
                log_debug0("F:%s, f:%s, L:%d, old stop timerID:%d\n", __FILE__, __func__, __LINE__, g_stop_timerID);
                if(g_stop_timerID != 0)
                    IOTTMR_delTimer(g_stop_timerID);
                g_stop_timerID = IOTTMR_addTimer(timer_event_cb, &g_timer_evtType, diffTime, 1);
                log_debug0("F:%s, f:%s, L:%d, new stop timerID:%d\n", __FILE__, __func__, __LINE__, g_stop_timerID);
            }
            log_debug0("F:%s, f:%s, L:%d, timeout %d seconds later\n", __FILE__, __func__, __LINE__, diffTime);
            iots_strcats(tts, tts_len, "设置成功我会在");
            iots_strcats(tts, tts_len - iots_strlen(tts), time_str);
            if (eventType == ShutdownEvt)
                iots_strcats(tts, tts_len - iots_strlen(tts), "关机");
            else if (eventType == StopEvt)
                iots_strcats(tts, tts_len - iots_strlen(tts), "停止");
        }
    } else {
        log_error("F:%s, f:%s, L:%d, unsupported tiemMode:%s\n", __FILE__, __func__, __LINE__, timestr);
        return E_FAILED;
    }
    return E_SUCCESS;
}

/* *
 * 定时关闭服务
 *@param[in]  req  命令指针
 *@param[in]  parm 时间、操作
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_req_handle_stop_service(CommandRequest *req, cJSON *param)
{
    int ret = E_FAILED;
    char _tts[128];
    cJSON *dateItem = NULL, *timeItem = NULL;
    log_debug0("F:%s, f:%s, L:%d\n", __FILE__, __func__, __LINE__);

    memset(_tts, 0, sizeof(_tts));

    //获取时间
    dateItem = cJSON_GetObjectItem(param, NLU_ALARM_DATE);
    timeItem = cJSON_GetObjectItem(param, NLU_ALARM_TIME);
    if (timeItem != NULL && timeItem->valuestring != NULL) {
        g_doItNow = 0;
        if (dateItem != NULL && dateItem->valuestring != NULL)
            ret = device_timed_event_handle(StopEvt, dateItem->valuestring, timeItem->valuestring, _tts, sizeof(_tts));
        else
            ret = device_timed_event_handle(StopEvt, NULL, timeItem->valuestring, _tts, sizeof(_tts));
    }
    if (ret == E_FAILED) {
        iots_strcpys(_tts, sizeof(_tts), "设置失败");
    } else if (g_doItNow == 1) {
    //定时停止时间点发生在过去，并且在过去一分钟之内
        Service_ServiceStop_ForDevice();
        iots_strcpys(_tts, sizeof(_tts), "已停止");
    }
    log_debug0("F:%s, f:%s, L:%d, [TIMEKEY] start playTTS:%s\n", __FILE__, __func__, __LINE__, _tts);
    CommandResponse_PlayTTS(req, _tts);
    if(req->source_serviceID == SERVICEID_VOICE) {
        cJSON_DeleteItemFromObject(req->data, NLU_REPLY);
        cJSON_AddStringToObject(req->data, NLU_REPLY, _tts);
    }
    return ret;
}

/* *
 * 关机/重启/定时关机
 *@param[in]  req  命令指针
 *@param[in]  item 关机/重启/定时关机
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_req_handle_shutdown(CommandRequest *req, cJSON *param)
{
    const int confirm_timeout = 20;
    char _tts[128];
    cJSON *item = NULL, *dateItem = NULL, *timeItem = NULL;

    log_assert(req != NULL && param != NULL);
    memset(_tts, 0, sizeof(_tts));

    item = cJSON_GetObjectItem(param, NLU_DISPATCH_OPERATION);
    if (item == NULL || item->valuestring == NULL) {
        log_error("F:%s, f:%s, L:%d, get item failed\n", __FILE__, __func__, __LINE__);
        return E_FAILED;
    }

    log_debug0("F:%s, f:%s, L:%d, operation %s\n", __FILE__, __func__, __LINE__, item->valuestring);
    if (strcmp(item->valuestring, NLU_DISPATCH_OPERATION_POWEROFF) == 0) {
    //关机
        dateItem = cJSON_GetObjectItem(param, NLU_ALARM_DATE);
        timeItem = cJSON_GetObjectItem(param, NLU_ALARM_TIME);
        if (timeItem != NULL && timeItem->valuestring != NULL) {
        //定时关机
            int ret = E_FAILED;
            g_doItNow = 0;
            if (dateItem != NULL && dateItem->valuestring != NULL)
                ret = device_timed_event_handle(ShutdownEvt, dateItem->valuestring, timeItem->valuestring, _tts, sizeof(_tts));
            else
                ret = device_timed_event_handle(ShutdownEvt, NULL, timeItem->valuestring, _tts, sizeof(_tts));

            if (ret == E_SUCCESS && g_doItNow == 1) {
            //定时关机时间点发生在过去，并在过去一分钟之内
                iots_sprintfs(_tts, sizeof(_tts), "请确认关机");
                g_poweroff_confirm = time(NULL) + confirm_timeout;
                log_debug0("F:%s, f:%s, L:%d, confirm poweroff start %d\n", __FILE__, __func__, __LINE__, g_poweroff_confirm);
                log_debug0("F:%s, f:%s, L:%d, [TIMEKEY] start mutiDialogue:%s\n", __FILE__, __func__, __LINE__, _tts);
                Service_StartMultiDialogue(g_device_serviceinfo.handle, req, _tts);
                return E_SUCCESS;
            } else if (ret == E_FAILED) {
                iots_strcpys(_tts, sizeof(_tts), "设置失败");
            }
            log_debug0("F:%s, f:%s, L:%d, [TIMEKEY] start playTTS:%s\n", __FILE__, __func__, __LINE__, _tts);
            CommandResponse_PlayTTS(req, _tts);
        } else {
        //非定时关机
            iots_sprintfs(_tts, sizeof(_tts), "请确认关机");
            g_poweroff_confirm = time(NULL) + confirm_timeout;
            log_debug0("F:%s, f:%s, L:%d, confirm poweroff start %d\n", __FILE__, __func__, __LINE__, g_poweroff_confirm);
            log_debug0("F:%s, f:%s, L:%d, [TIMEKEY] start mutiDialogue:%s\n", __FILE__, __func__, __LINE__, _tts);
            Service_StartMultiDialogue(g_device_serviceinfo.handle, req, _tts);
        }
    } else if (HARDWARE_SUPPORT_REBOOT && strcmp(item->valuestring, NLU_DISPATCH_OPERATION_REBOOT) == 0) {
    //重启
        log_assert(g_reboot_confirm == 0);
#if 0
        iots_sprintfs(_tts, sizeof(_tts), "请确认重启");
        g_reboot_confirm = time(NULL) + confirm_timeout;
        log_debug0("confirm reboot start %d\n", g_reboot_confirm);
        Service_StartMultiDialogue(g_device_serviceinfo.handle, req, _tts);
#else
        iots_sprintfs(_tts, sizeof(_tts), "重启暂不支持 请手动操作");
        log_debug0("F:%s, f:%s, L:%d, [TIMEKEY] start playTTS:%s\n", __FILE__, __func__, __LINE__, _tts);
        CommandResponse_PlayTTS(req, _tts);
#endif
    } else {
        log_warning("F:%s, f:%s, L:%d, unsupport operation %s\n", __FILE__, __func__, __LINE__, item->valuestring);
        iots_sprintfs(_tts, sizeof(_tts), "不支持此操作");
        log_debug0("F:%s, f:%s, L:%d, [TIMEKEY] start playTTS:%s\n", __FILE__, __func__, __LINE__, _tts);
        CommandResponse_PlayTTS(req, _tts);
    }
    if(req->source_serviceID == SERVICEID_VOICE) {
        cJSON_DeleteItemFromObject(req->data, NLU_REPLY);
        cJSON_AddStringToObject(req->data, NLU_REPLY, _tts);
    }
    return E_SUCCESS;
}

/* *
 * 取消关机
 *@param[in]  req  命令指针
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_req_handle_cancel_shutdown(CommandRequest *req)
{
    char _tts[128];
    memset(_tts, 0, sizeof(_tts));
    log_debug0("F:%s, f:%s, L:%d, cancel shutdown timerID=%d\n", __FILE__, __func__, __LINE__, g_shutdown_timerID);

    if (g_shutdown_timerID != 0) {
        IOTTMR_delTimer(g_shutdown_timerID);
        g_shutdown_timerID = 0;
        g_shutdown_time = 0;
        iots_sprintfs(_tts, sizeof(_tts), "取消成功");
    } else {
        iots_sprintfs(_tts, sizeof(_tts), "当前没有关机任务呦");
    }
    log_debug0("F:%s, f:%s, L:%d, [TIMEKEY] start playTTS:%s\n", __FILE__, __func__, __LINE__, _tts);
    CommandResponse_PlayTTS(req, _tts);

    if(req->source_serviceID ==  SERVICEID_VOICE) {
        cJSON_DeleteItemFromObject(req->data, NLU_REPLY);
        cJSON_AddStringToObject(req->data, NLU_REPLY, _tts);
    }
    return E_SUCCESS;
}

#ifdef PLATFORM_CHATBOT_DOULE
#include "usb_send_data.h"

static int g_doule_light = 0;
static char g_path_light[MAX_PATH];

/* *
 * 设置豆乐的灯
 *@param[in]  data  0-100
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_set_light(int data)
{
    int ret;

    data = data * 255 / 100;
    if (data > 255) data = 255;
    if (data < 0) data = 0;
    log_debug0("F:%s, f:%s, L:%d usb_send_data = %d \n", __FILE__, __func__, __LINE__, data);
    ret = usb_send_data(data);
    log_debug0("F:%s, f:%s, L:%d usb_send_data ret = %d\n", __FILE__, __func__, __LINE__, ret);

    if (ret >= 0)
        return E_SUCCESS;
    else
        return E_FAILED;
}

/* *
 * 开灯，需要读取上次的值
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_open_light()
{
    int data;
    // 从文件读取
    if (IOTFile_ReadFile(g_path_light, sizeof(data), (char *)&data) == E_SUCCESS)
    {
        log_debug0("F:%s, f:%s, L:%d, read = %d\n", __FILE__, __func__, __LINE__, data);
        if (data > 0 && data <= 100)
            g_doule_light = data;
    }

    if (g_doule_light == 0)
        g_doule_light = 70;
    data = g_doule_light;
    return device_set_light(data);
}

/* *
 * 关灯
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_close_light()
{
    return device_set_light(0);
}

/* *
 * 固定亮度设置
 *@param[in]  data  0-100
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_change_light(int data)
{
    int ret;
    int light;

    light = usb_get_lightness();
    log_debug0("F:%s, f:%s, L:%d, get light = %d\n", __FILE__, __func__, __LINE__, light);
    if (light <= 0) {
        log_debug0("F:%s, f:%s, L:%d, do not set!!\n", __FILE__, __func__, __LINE__);
        return E_FAILED;
    }
    if (data > 100) data = 100;
    if (data <= 0) data = 1;

    ret = device_set_light(data);
    if (ret == E_SUCCESS && data > 0) {
        g_doule_light = data;
        // 存到文件
        if (IOTFile_WriteBFile(g_path_light, (char *)&data, sizeof(data)) == E_SUCCESS)
        {
            log_debug0("F:%s, f:%s, L:%d, write = %d\n", __FILE__, __func__, __LINE__, data);
            if (data > 0 && data <= 100)
                g_doule_light = data;
        }
    }
    return ret;
}

/* *
 * 亮度+/-
 *@param[in]  mode  1:亮度增加；-1:亮度减少
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_adjust_light(int mode)
{
    static const int level[] = {0, 11, 40, 70, 100};
    int index = 0;
    int data = g_doule_light;
    log_debug0("F:%s, f:%s, L:%d source data = %d \n", __FILE__, __func__, __LINE__, data);

    if (mode == 1)  // 增加
    {
        for (index = 0; index < (sizeof(level) / sizeof(level[0]) - 1); index ++)
        {
            if (data >= level[index] && data < level[index + 1])
            {
                data = level[index + 1];
                break;
            }
        }
    }
    else // 降低
    {
        for (index = 0; index < (sizeof(level) / sizeof(level[0]) - 1); index ++)
        {
            if (data > level[index] && data <= level[index + 1])
            {
                data = level[index];
                break;
            }
        }
    }
    log_debug0("F:%s, f:%s, L:%d taget data = %d \n", __FILE__, __func__, __LINE__, data);

    return device_change_light(data);
}

/* *
 * 设置设置豆乐的灯
 *@param[in]  req  命令指针
 *@param[in]  param 参数
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_req_handle_lightadjust(CommandRequest *req, cJSON *param)
{
    cJSON *item;
    int ret;

    item = cJSON_GetObjectItem(param, NLU_SMARTHOME_ACTION);
    if(item != NULL && item->valuestring != NULL) {
        if (strcmp(item->valuestring, NLU_SMARTHOME_ACTION_ON) == 0) {
            ret = device_open_light();
            if (ret == E_SUCCESS)
                CommandResponse_PlayTTS(req, "已开灯");
            goto out;
        } else if (strcmp(item->valuestring, NLU_SMARTHOME_ACTION_OFF) == 0) {
            ret = device_close_light();
            if (ret == E_SUCCESS)
                CommandResponse_PlayTTS(req, "已关灯");
            goto out;
        }
    }

    item = cJSON_GetObjectItem(param, NLU_SMARTHOME_BRIGHTNESS);
    if(item != NULL) {
        if(item->type == cJSON_String) {
            if (strcmp(item->valuestring, "+") == 0) {
                device_adjust_light(1);
                goto out;
            } else if (strcmp(item->valuestring, "-") == 0) {
                device_adjust_light(-1);
                goto out;
            }
        } else if(item->type == cJSON_Number) {
            device_change_light(item->valueint);
            goto out;
        }
    }
    return E_FAILED;
out:
    return E_SUCCESS;
}
#endif
/* *
 * 设置音量
 *@param[in]  req  命令指针
 *@param[in]  item 音量大小
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_req_handle_voladjust(CommandRequest *req, cJSON *item)
{
    SpeakerData *speaker_data = Speaker_GetData();
    const char *mp3 = NULL;
    char _tts[128];
    int retval = E_FAILED;
    int v, tmpv;
    int sourceID;
    VolCtrl volctrl = VolumeNormal;
    log_assert(req != NULL && item != NULL);
    log_assert(speaker_data != NULL);

    sourceID = req->source_serviceID;

    v = speaker_data->main_volume;
    memset(_tts, 0, sizeof(_tts));
    if(item->type == cJSON_String) {
        if(strcmp(item->valuestring, "+") == 0) {
        //声音大点
            volctrl = VolumeAdd;
            v += 10;
        } else if(strcmp(item->valuestring, "-") == 0) {
        //声音小点
            volctrl = VolumeSub;
            v -= 10;
            if (sourceID == SERVICEID_VOICE && v < 5) {
                log_debug0("F:%s, f:%s, L:%d, sourceID = SERVICEID_VOICE, min volume is 5\n", __FILE__, __func__, __LINE__);
                v = 5;
            }
        } else if(strcmp(item->valuestring, "max") == 0) {
        //最大声音
            volctrl = VolumeMax;
            v = VOLUME_VALUE_MAX;
        } else if(strcmp(item->valuestring, "min") == 0) {
        //最小声音
            volctrl = VolumeMin;
            v = VOLUME_VALUE_MIN;
        } else if (strcmp(item->valuestring, "mute") == 0) {
        //静音
            volctrl = VolumeMute;
        } else if(strcmp(item->valuestring, "unmute") == 0
                || strcmp(item->valuestring, "open") == 0) {
        //取消静音
            volctrl = VolumeUnmute;
        } else {
            if(iots_sscanf(item->valuestring, "+%u", &tmpv) == 1) {
            //音量加xx
                volctrl = VolumeAddXX;
                v += tmpv;
            } else if (iots_sscanf(item->valuestring, "-%d", &tmpv) == 1) {
            //音量减xx
                volctrl = VolumeSubXX;
                v -= tmpv;
            } else if (iots_sscanf(item->valuestring, "%d", &tmpv) == 1) {
            //音量设为xx
                volctrl = VolumeXX;
                v = tmpv;
            } else {
                log_error("F:%s, f:%s, L:%d, unknow volume value=%s\n", __FILE__, __func__, __LINE__, item->valuestring);
                goto failed;
            }
        }
    } else if(item->type == cJSON_Number) {
        volctrl = VolumeXX;
        v = item->valueint;
    } else {
        log_error("F:%s, f:%s, L:%d, unknow volume, json type=%d\n", __FILE__, __func__, __LINE__, item->type);
        goto failed;
    }

    log_debug0("F:%s, f:%s, L:%d, source_serviceID=0x%x,volctrl=%d\n", __FILE__, __func__, __LINE__, req->source_serviceID, volctrl);
    if (volctrl >= VolumeAdd && volctrl <= VolumeMin) {
    //音量调节
        if(v > VOLUME_VALUE_MAX) v = VOLUME_VALUE_MAX;
        if(v < VOLUME_VALUE_MIN) v = VOLUME_VALUE_MIN;
        log_assert(v <= VOLUME_VALUE_MAX && v >= VOLUME_VALUE_MIN);
        //语音把音量设为0时 先播报在回调中设置音量,其他情况先设置音量再播报
        if (v > 0 || sourceID != SERVICEID_VOICE) {
            vol_adjust_ctrl(v);
        }
        iots_sprintfs(_tts, sizeof(_tts), "音量已设置为%u", v);
    } else if (volctrl == VolumeMute) {
    //静音, 静音操作接口在播报TTS回调中调用
        iots_sprintfs(_tts, sizeof(_tts), "已静音");
        mp3 = PROMPT_TONE_SERVICEMUTE;
    } else if (volctrl == VolumeUnmute) {
    //取消静音
        iots_sprintfs(_tts, sizeof(_tts), "已取消静音");
        mp3 = PROMPT_TONE_SERVICEUNMUTE;
        vol_mute_ctrl(0);
    }

    if(req->source_serviceID == SERVICEID_VOICE) {
        log_debug0("F:%s, f:%s, L:%d, [TIMEKEY] start playTTS:%s\n", __FILE__, __func__, __LINE__, _tts);
        //静音操作或音量设为0 先播报TTS再做操作
        if (volctrl == VolumeMute || v == 0)
        {
            if (volctrl == VolumeMute)
                g_CBCtrl = SetMute;
            else
                g_CBCtrl = SetZero;
            if (mp3 != NULL)
                CommandResponse_PlayFileWithCB(req, mp3, play_tts_endcb, &g_CBCtrl);
            else
                CommandResponse_PlayTTSWithCB(req, _tts, play_tts_endcb, &g_CBCtrl);
        }
        else
        {
            if (mp3 != NULL)
                CommandResponse_PlayFile(req, mp3);
            else
                CommandResponse_PlayTTS(req, _tts);
        }
        cJSON_DeleteItemFromObject(req->data, NLU_REPLY);
        cJSON_AddStringToObject(req->data, NLU_REPLY, _tts);
    }

    retval = E_SUCCESS;
failed:
    return retval;
}

/* *
 * set volume mute
 *@param[in]  req  命令指针
 *@param[in]  item 设置方式
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_req_handle_volmute(CommandRequest *req, cJSON *item)
{
    UINT32 volume_mute;

    log_assert(req != NULL && item != NULL);
    volume_mute = (item->valueint)?1:0;
    vol_mute_ctrl(volume_mute);
    return E_SUCCESS;
}

/* *
 * set mic mute
 *@param[in]  req  命令指针
 *@param[in]  item 设置方式
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_req_handle_micmute(CommandRequest *req, cJSON *item)
{
    SpeakerData *speaker_data = Speaker_GetData();
    UINT32 mic_mute;

    log_assert(req != NULL && item != NULL);
    log_assert(speaker_data != NULL);

    mic_mute = (item->valueint)?1:0;
    log_debug0("F:%s, f:%s, L:%d, from service(0x%x) set mic mute(%d->%d)\n",
            __FILE__, __func__, __LINE__, req->source_serviceID, speaker_data->is_micmute, mic_mute);
    if(speaker_data->is_micmute != mic_mute) {
        speaker_data->is_micmute = mic_mute;
        Speaker_SetMicMute(speaker_data->is_micmute);
        Service_CallEventCB(SERVICE_EVENT_MICMUTE_CHANGED, &speaker_data->is_micmute);
    }
    return E_SUCCESS;
}

/* *
 * set voice type
 *@param[in]  req  命令指针
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
static int device_req_handle_voicetype(CommandRequest *req, cJSON *item)
{
    SpeakerData *speaker_data = Speaker_GetData();
    UINT32 tts_voicetype;

    log_assert(req != NULL && item != NULL);
    log_assert(speaker_data != NULL);

    tts_voicetype = (UINT32)item->valueint;
    if(speaker_data->tts_voicetype != tts_voicetype) {
        log_debug0("F:%s, f:%s, L:%d, from service(0x%x) set voice type(%d->%d)\n",
                __FILE__, __func__, __LINE__, req->source_serviceID, speaker_data->tts_voicetype, tts_voicetype);
        if(VoiceEngineSetVoiceType(tts_voicetype) == E_SUCCESS) {
            speaker_data->tts_voicetype = tts_voicetype;
            Service_CallEventCB(SERVICE_EVENT_VOICETYPE_CHANGED, &speaker_data->tts_voicetype);
        } else {
            log_error("F:%s, f:%s, L:%d, set voice type failed\n", __FILE__, __func__, __LINE__);
        }
    } else {
        log_debug0("F:%s, f:%s, L:%d, from service(0x%x) ignore set voice type(%d) unchanged\n",
                __FILE__, __func__, __LINE__, req->source_serviceID, speaker_data->tts_voicetype);
    }
    return E_SUCCESS;
}

/* *
 * 处理中控命令
 *@param[in]  req  命令指针
 *@return     E_IN_PROCESS 成功缓存命令，等待response再删除
 *            E_SUCCESS 成功执行完命令，可以删除了
 *            E_FAILED 失败
 *
 * */
static int device_req_handle(CommandRequest *req)
{
    cJSON *param;
    cJSON *item;
    int valuechanged = 0;
    log_debug0("F:%s, f:%s, L:%d\n", __FILE__, __func__, __LINE__);

	if(req->target_serviceID != SERVICEID_DEIVCEDEP) {
		return E_FAILED;
	}

    if(req->source_serviceID == SERVICEID_VOICE) {
        log_assert(req->req_errno == E_SUCCESS);
        param = cJSON_GetObjectItem(req->data, NLU_PARAM);
        if(param == NULL) {
            log_error("F:%s, f:%s, L:%d get item param failed\n", __FILE__, __func__, __LINE__);
            return E_FAILED;
        }
    } else {
        param = req->data;
    }


#ifdef PLATFORM_CHATBOT_DOULE
    item = cJSON_GetObjectItem(param, NLU_CTRL_DEVICENAME);
    if(item != NULL && item->valuestring != NULL && strcmp(item->valuestring, NLU_SMARTHOME_DEVICENAME_BULB) == 0) {
        device_req_handle_lightadjust(req, param);
        goto out;
    }
#endif
    item = cJSON_GetObjectItem(param, NLU_DISPATCH_VOLUME);
    if(item != NULL) {
    // set volume
        device_req_handle_voladjust(req, item);
        valuechanged++;
        goto out;
    }

    item = cJSON_GetObjectItem(param, NLU_DISPATCH_OPERATION);
    if(item != NULL && item->valuestring != NULL) {
    // poweroff, reboot, cancel poweroff, stop service
        if (strcmp(item->valuestring, NLU_DISPATCH_OPERATION_POWEROFF) == 0
                || strcmp(item->valuestring, NLU_DISPATCH_OPERATION_REBOOT) == 0) {
            device_req_handle_shutdown(req, param);
            goto out;
        } else if (strcmp(item->valuestring, NLU_DISPATCH_OPERATION_CANCEL_POWEROFF) == 0) {
            device_req_handle_cancel_shutdown(req);
            goto out;
        } else if (strcmp(item->valuestring, NLU_CTRL_OPERATION_STOP) == 0
                || strcmp(item->valuestring, NLU_CTRL_OPERATION_PAUSE) == 0) {
            device_req_handle_stop_service(req, param);
            goto out;
        } else {
            log_warning("F:%s, f:%s, L:%d, unsupported opreation:%s\n", __FILE__, __func__, __LINE__, item->valuestring);
            goto out;
        }
    }

    item = cJSON_GetObjectItem(param, NLU_DISPATCH_VOLUMEMUTE);
    if(item != NULL) {
    // set volume mute
        device_req_handle_volmute(req, item);
        valuechanged++;
        goto out;
    }

    item = cJSON_GetObjectItem(param, NLU_DISPATCH_MICMUTE);
    if(item != NULL) {
    // set mic mute
        device_req_handle_micmute(req, item);
        valuechanged++;
        goto out;
    }

    item = cJSON_GetObjectItem(param, NLU_DISPATCH_VOICETYPE);
    if(item != NULL) {
    // set voicetype
        device_req_handle_voicetype(req, item);
        valuechanged++;
        goto out;
    }

    {
    char *str = cJSON_Print(param);
    log_error("F:%s, f:%s, L:%d, unkown dispatch request param=%s\n", __FILE__, __func__, __LINE__, str);
    cJSON_MFree(str);str = NULL;
    }

out:
    if(valuechanged != 0) {
        Speaker_UpdateDataStatus(Speaker_GetData());
    }
    return E_SUCCESS;
}

/* *
 * 中控服务初始化
 *@param[in]  handle  服务句柄
 *@return     E_SUCCESS 成功
 *            E_FAILED 失败
 *
 * */
int Service_Init(SVCHandle handle)
{
	memset(&g_device_serviceinfo, 0, sizeof(ServiceInfo));
	g_device_serviceinfo.serviceID = SERVICEID_DEIVCEDEP;
	g_device_serviceinfo.vendorID  = VENDORID_Lenovo;
    g_device_serviceinfo.handle    = handle;

#ifdef PLATFORM_CHATBOT_DOULE
    iots_sprintfs(g_path_light, sizeof(g_path_light), "%s/light.def", Service_UsrdataDirectory());
#endif
	if (ServiceRegister(handle, &g_device_serviceinfo) == E_FAILED) {
        log_error("F:%s, f:%s, L:%d, register 0x%x service failed!\n", __FILE__, __func__, __LINE__, SERVICEID_DEIVCEDEP);
        return E_FAILED;
	}
	return E_SUCCESS;
}

/* *
 * 中控服务结束
 *
 * */
void Service_Destroy(void)
{
    ServiceUnregister(g_device_serviceinfo.handle, &g_device_serviceinfo);
	memset(&g_device_serviceinfo, 0, sizeof(ServiceInfo));
}

/* *
 * 中控服务开始
 *@param[in]  cmdreq  命令指针
 *@return     E_IN_PROCESS 成功缓存命令，等待response再删除
 *            E_SUCCESS 成功执行完命令，可以删除了
 *            E_FAILED 失败
 *
 * */
int Service_Start(CommandRequest *cmdreq)
{
	log_debug0("%s(%d): service=0x%x, errno=%d, p=%d/r%d\n", __func__, __LINE__,
        ((cmdreq==NULL)?0:cmdreq->target_serviceID), ((cmdreq==NULL)?0:cmdreq->req_errno), g_poweroff_confirm, g_reboot_confirm);
    if(cmdreq == NULL || cmdreq->req_errno == E_NO_DATA) {
        g_poweroff_confirm = 0;
        g_reboot_confirm   = 0;
        return E_SUCCESS;
    }

	if(cmdreq->target_serviceID != SERVICEID_DEIVCEDEP &&
            cmdreq->target_serviceID != SERVICEID_CTRL) {
        log_warning("F:%s, f:%s, L:%d, wrong target is 0x%x\n", __FILE__, __func__, __LINE__, cmdreq->target_serviceID);
		return E_FAILED;
	}

    if(cmdreq->source_serviceID == SERVICEID_VOICE) {
        if(cmdreq->req_errno != E_SUCCESS) {
            // failed request handle, //remove from this service later......
            g_poweroff_confirm = 0;
            g_reboot_confirm   = 0;
            device_req_handle_failedrequest(cmdreq);
            return E_SUCCESS;
        }
    }
    log_assert(cmdreq->data != NULL);

    if(g_poweroff_confirm != 0) {
        if(device_req_handle_shutdown_confirm(cmdreq, 0) == E_SUCCESS) {
            g_poweroff_confirm = 0;
            return E_SUCCESS;
        }
        g_poweroff_confirm = 0;
    }

    if(g_reboot_confirm != 0) {
        if(device_req_handle_shutdown_confirm(cmdreq, 1) == E_SUCCESS) {
            g_reboot_confirm = 0;
            return E_SUCCESS;
        }
        g_reboot_confirm = 0;
    }

    log_assert(g_reboot_confirm   == 0);
    log_assert(g_poweroff_confirm == 0);

    return device_req_handle(cmdreq);
}

