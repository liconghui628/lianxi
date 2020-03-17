#!/bin/bash
echo FileName=$1 VoiceType=$2 Speed=$3 Text=$4
if [ $# != 4 ] ; then 
    echo "Param error!" 
    echo "CALL: $0 [FileName] [VoiceType] [Speed] [Text]" 
    exit 1; 
fi
if [ $2 != "qianranf" ] && [ $2 != "xjingf" ] && [ $2 != "xijunm" ] && [ $2 != "zhilingf" ] && [ $2 != "boy" ] && [ $2 != "qiumum" ]
then
    echo Unsupported VoiceType, Must be [qianranf xjingf xijunm zhilingf boy qiumum]
    exit 1;
fi
curl -o $1 -d "{\"context\":{\"productId\":\"278573569\"},\"request\":{\"requestId\":\"3123421212gg2\",\"audio\":{\"audioType\":\"mp3\",\"mp3Quality\":\"high\",\"sampleRate\":16000,\"channel\":1,\"sampleBytes\":2},\"tts\":{\"text\":\"$4\",\"textType\":\"text\",\"speed\":$3, \"voiceId\":\"$2\"}}}" 'https://tts.dui.ai/runtime/v2/synthesize?productId=278573569&apikey=5b04d260b6a84adca46c21576bb19c07'
