// 递归删除字符串中特定的字符
void delete_str_characters(char *str, char ch )
{
    char *p1 = NULL, *p2 = NULL;
    p1 = strchr(str, ch);
    if (p1 != NULL) {
        p2 = p1;
        p1 ++;
        delete_str_characters(p1, ch);
        memmove(p2, p1, iots_strlen(p1) + 1);
    } else {
        return;
    }
}

// 递归删除两个字符之间的文本
void delete_str_between_characters(char *str, char ch1, char ch2)
{
    char *p1 = NULL, *p2 = NULL;
    // 删除ch1和ch2之间的字符
    p1 = strchr(str, ch1);
    if (p1 != NULL) {
        p2 = strchr(p1, ch2);
        if (p2 != NULL) {
            p2 ++;
            delete_str_between_characters(p2, ch1, ch2);
            memmove(p1, p2, iots_strlen(p2) + 1);
        }
    }
}
