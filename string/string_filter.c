#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//把长字文本分割成多个完整的短文本，每个短文本1024字节左右
//如果一个文本1200字节，则分成两个短文本，每个600字节左右
//如果一个文本3000字节，则分成两个短文本，每个1000字节左右
void cut_content_to_units(char *content, char **units, int unitCnt, char *split)
{
    int i = 0, arrCnt = 0, contentLen = 0;
    char *start = NULL, *p1 = NULL;
    if (content == NULL || units == NULL || unitCnt <= 0 || split == NULL) {
        return;
    }

    //尽可能把字符串平均切割
    contentLen = strlen(content);
    arrCnt = contentLen / 1024 + 1;
    arrCnt = arrCnt < unitCnt ? arrCnt : unitCnt;

    start = content;
    for (i = 0; i < arrCnt; i ++) {
        p1 = strstr(start, split);
        units[i] = start;
        while (p1 && (p1 - start < contentLen/arrCnt)) {
            p1 = strstr(p1 + strlen(split), split);
        }
        if (p1) {
            *p1 = '\0';
            start = p1 + strlen(split);
        } else {
            break;
        }
    }

    // print
    i = 0;
    while (units[i]) {
        printf("F:%s,f:%s, L:%d [%d]:%s\n", __FILE__, __func__, __LINE__,i,units[i]);
        i ++;
    }
}

// 递归删除字符串中特定的字符串，可以完全取代delete_str_character()
void delete_str_characters(char *str, char *delstr )
{
    char *p1 = NULL, *p2 = NULL;
    p1 = strstr(str, delstr);
    if (p1 != NULL) {
        p2 = p1;
        p1 += strlen(delstr);
        delete_str_characters(p1, delstr);
        memmove(p2, p1, strlen(p1) + 1);
    }
}

// 递归删除字符串中特定的字符,可以被delete_str_characters()取代
void delete_str_character(char *str, char ch )
{
    char *p1 = NULL, *p2 = NULL;
    p1 = strchr(str, ch);
    if (p1 != NULL) {
        p2 = p1;
        p1 ++;
        delete_str_character(p1, ch);
        memmove(p2, p1, strlen(p1) + 1);
    } 
}

// 递归删除两个字符之间的文本
void delete_str_between_characters(char *str, char ch1, char ch2)
{
    char *p1 = NULL, *p2 = NULL;
    // 删除ch1和ch2之间的字符
    p1 = strchr(str, ch1);
    if (p1 != NULL) {
        p2 = strchr(p1+1, ch2);
        if (p2 != NULL) {
            p2 ++;
            delete_str_between_characters(p2, ch1, ch2);
            memmove(p1, p2, strlen(p2) + 1);
        }
    }
}

int main()
{
    char str[] = "    a bc de f  ghi     jk               lm n   ";
    printf("str:%s\n",str);
    delete_str_character(str, ' ');
    printf("str:%s\n",str);
    delete_str_characters(str, "abc");
    printf("str:%s\n",str);
    delete_str_characters(str, "lmn");
    printf("str:%s\n",str);

    return 0;
}
