#!/bin/bash
voice_array=(qianranf \
        xjingf \
        xijunm \
        zhilingf \
        boy \
        qiumum)

mp3_sys_array=(Hello.mp3 \
        HelloXiaole.mp3 \
        NetWorkConfigErrTryAgain.mp3 \
        NetWorkIsConnecting.mp3 \
        SetNetWorkByApp.mp3 \
        SoftAp.mp3)

tts_sys_array=("网络连接成功，请试着对音箱说，你好联想" \
        "网络连接成功，请试着对音箱说，小乐小乐" \
        "网络连接失败，请重新配置" \
        "正在连接，请稍等" \
        "您好，感谢您使用联想智能音箱，请扫描音箱底部的二维码，下载智慧联想客户端,并根据手机客户端的引导配置网络" \
        "已进入无线热点模式")

mp3_doulesys_array=(Hello.mp3 \
        NetWorkConfigErrTryAgain.mp3 \
        NetWorkIsConnecting.mp3 \
        SetNetWorkByApp.mp3 \
        SoftAp.mp3)

tts_doulesys_array=("网络连接成功，请试着对音箱说，你好逗乐" \
        "网络连接失败，请重新配置" \
        "正在连接，请稍等" \
        "您好，感谢您使用联想智能音箱，请扫描音箱底部的二维码，下载智慧联想客户端,并根据手机客户端的引导配置网络" \
        "已进入无线热点模式")

mp3_array=(BluetoothBusy.mp3 \
        BluetoothCloseed.mp3 \
        BluetoothConnect.mp3 \
        BluetoothDisconnect.mp3 \
        BluetoothOpened.mp3 \
        CuiError.mp3 \
        DevOff.mp3 \
        InitFailedReboot.mp3 \
        NetWorkNotConnected.mp3 \
        Triggernetstatus.mp3)

tts_array=("音箱已经与其他设备建立蓝牙连接，请断开现有的连接，再进行重试" \
        "蓝牙已关闭" \
        "蓝牙连接成功" \
        "蓝牙连接断开" \
        "蓝牙已打开" \
        "网络开小差了，请再说一遍" \
        "正在关机，下次见" \
        "初始化失败，请重启音箱" \
        "网络未连接，请重新配置" \
        "你的网络环境有点问题，检查一下网络后再试试")

echo -e "\n开始清除历史数据......\n\n"
for file in ./*
do
    if [ $file != $0 ]; then
        rm -rf $file
    fi
done
echo -e "done!!!\n\n"

echo -e "开始生成 MINI、G1配网音频......\n\n"
if [ ! -d "sys" ];then
    mkdir -p sys
fi
for i in ${!mp3_sys_array[*]};do
    echo FileName:sys/${mp3_sys_array[$i]} TTS:${tts_sys_array[$i]}
    curl -o sys/${mp3_sys_array[$i]} -d "{\"context\":{\"productId\":\"278573569\"},\"request\":{\"requestId\":\"3123421212gg2\",\"audio\":{\"audioType\":\"mp3\",\"mp3Quality\":\"high\",\"sampleRate\":16000,\"channel\":1,\"sampleBytes\":2},\"tts\":{\"text\":\"${tts_sys_array[$i]}\",\"textType\":\"text\",\"speed\":1, \"voiceId\":\"zhilingf\"}}}" 'https://tts.dui.ai/runtime/v2/synthesize?productId=278573569&apikey=5b04d260b6a84adca46c21576bb19c07'
done
echo -e "done!!!\n\n"

echo -e "开始生成Doule配网音频......\n\n"
if [ ! -d "sys_doule" ];then
    mkdir -p sys_doule
fi
for i in ${!mp3_doulesys_array[*]};do
    echo FileName:sys_doule/${mp3_sys_array[$i]} TTS:${tts_sys_array[$i]}
    curl -o sys_doule/${mp3_doulesys_array[$i]} -d "{\"context\":{\"productId\":\"278573569\"},\"request\":{\"requestId\":\"3123421212gg2\",\"audio\":{\"audioType\":\"mp3\",\"mp3Quality\":\"high\",\"sampleRate\":16000,\"channel\":1,\"sampleBytes\":2},\"tts\":{\"text\":\"${tts_doulesys_array[$i]}\",\"textType\":\"text\",\"speed\":1, \"voiceId\":\"qianranf\"}}}" 'https://tts.dui.ai/runtime/v2/synthesize?productId=278573569&apikey=5b04d260b6a84adca46c21576bb19c07'
done
echo -e "done!!!\n\n"

echo -e "开始生成系统音频......\n\n"
for i in ${voice_array[*]};do  
    echo $i
    if [ ! -d $i ];then
        mkdir -p $i
    fi
    for j in ${!mp3_array[*]}; do
        echo FileName:$i/${mp3_array[$j]} TTS:${tts_array[$j]}
        curl -o $i/${mp3_array[$j]} -d "{\"context\":{\"productId\":\"278573569\"},\"request\":{\"requestId\":\"3123421212gg2\",\"audio\":{\"audioType\":\"mp3\",\"mp3Quality\":\"high\",\"sampleRate\":16000,\"channel\":1,\"sampleBytes\":2},\"tts\":{\"text\":\"${tts_array[$j]}\",\"textType\":\"text\",\"speed\":1, \"voiceId\":\"$i\"}}}" 'https://tts.dui.ai/runtime/v2/synthesize?productId=278573569&apikey=5b04d260b6a84adca46c21576bb19c07'
    done
done
echo -e "done!!!\n\n"

echo -e "开始打包音频数据 TTS.rar ......\n\n"
tar cf TTS.tar ${voice_array[@]} sys sys_doule
echo -e "done!!!\n\nSuccess!!!"
