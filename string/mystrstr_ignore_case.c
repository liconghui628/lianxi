#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* *
 * 对比两个字符是否相等（忽略大小写）
 *@param[in]  ch1, ch2   需要对比的字符
 *@return     0          相等
 *            -1         不相等
 * */
static int charcmp_nocase(char ch1, char ch2)
{
    //printf("ch1=%hhd,ch2=%hhd\n",ch1,ch2);
    if (ch1 == ch2){
        return 0;
    }
    else if (((ch1 >= 'A' && ch1 <= 'Z') || (ch1 >= 'a' && ch1 <= 'z'))
            && ((ch2 >= 'A' && ch2 <= 'Z') || (ch2 >= 'a' && ch2 <= 'z'))) {
        if (ch1 - ch2 == 'a' - 'A' || ch1 - ch2 == 'A' - 'a')
            return 0;
    }
    return (ch1 > ch2 ? 1 : -1) ;
}

/* *
 * 判断一个字符串中是否包含子串(忽略大小写)
 *@param[in]  str1      长字符串
 *@param[in]  str2      子串
 *@return     p1        字串开始的地址
 *            NULL      失败
 *
 * */
static char* strstr_nocase(char *str1, char *str2)
{
    char *p1 = NULL, *p2 = NULL, *ret = NULL;
    if (str1 == NULL || str2 == NULL) {
        printf("F:%s, f:%s, L:%d, param error!\n", __FILE__, __func__, __LINE__);
        return NULL;
    }
    printf("F:%s, f:%s, L:%d, str1:%s, str2:%s\n", __FILE__, __func__, __LINE__,str1, str2);
    p1 = str1;
    while (*p1 != '\0') {
        ret = p1;
        p2 = str2;
        while ( *p2 != '\0' && charcmp_nocase(*p1, *p2) == 0) {
            p2 ++;
            p1 ++;
        }
        if (*p2 == '\0')
            return ret;
        p1 = ret+1;
    }
    return NULL;
}

int main(void)
{
    char *str1 = "你好";
    char *str2 = "你好啊";
    char *str = strstr_nocase(str1, str2);
    if (str == NULL) {
        printf("str=NULL\n");
    } else {
        printf("str=%s\n",str);
    }
}
