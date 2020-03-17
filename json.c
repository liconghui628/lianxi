#include <string.h>
#include <stdio.h>
#include "json.h"

JSON* JSON_CreateObject(void) 
{
    return cJSON_CreateObject();
}

JSON* JSON_CreateArray(void)
{
    return cJSON_CreateArray();
}

int JSON_GetArraySize(const JSON *array)
{
    return cJSON_GetArraySize(array);
}

JSON* JSON_GetArrayItem(const JSON *array, int index)
{
    return cJSON_GetArrayItem(array, index);
}

JSON* JSON_GetObjectItem(const JSON * const object, const char * const string)
{
    return cJSON_GetObjectItem(object, string);
}

JSON* JSON_AddNumberToObject(JSON * const object, const char * const name, const double number)
{
    return cJSON_AddNumberToObject(object, name, number);
}

JSON* JSON_AddStringToObject(JSON * const object, const char * const name, const char * const string)
{
    return cJSON_AddStringToObject(object, name, string);
}

JSON* JSON_Duplicate(const JSON *item, int recurse)
{
    return cJSON_Duplicate(item, recurse);
}

JSON* JSON_Parse(const char *value)
{
    return cJSON_Parse(value);
}

char* JSON_Print(const JSON *item)
{
    return cJSON_Print(item);
}

void JSON_AddItemToArray(JSON *array, JSON *item)
{
    cJSON_AddItemToArray(array, item);
}

void JSON_AddItemToObject(JSON *object, const char *string, JSON *item)
{
    cJSON_AddItemToObject(object, string, item);
}

void JSON_Delete(JSON *c)
{
    cJSON_Delete(c);
}

int JSON_To_Struct(JSON *json, void *base_addr, StructMap *map, int map_cnt)
{
    int i = 0, j = 0;
    JSON *item = NULL, *objitem = NULL;

    if (json == NULL || base_addr == NULL || map == NULL || map_cnt < 1) 
    {
        return -1;
    }

    for (i = 0; i < map_cnt; i++)
    {
        item = JSON_GetObjectItem(json, map[i].key);
        if (item) 
        {
            printf("[%d]key=%s, offset=%d, type=%d, item->type=%d\n", i, map[i].key,map[i].offset,map[i].type,item->type);
            if (item->type == cJSON_String && map[i].type == JSON_String)       //string
            {
                if (item->valuestring)
                {
                    strcpy((char*)(base_addr + map[i].offset), item->valuestring);
                }
            }
            else if (item->type == cJSON_Number &&  map[i].type == JSON_Int)    // int 
            {
                *((int*)(base_addr + map[i].offset)) = item->valueint;
            }
            else if (item->type == cJSON_Number && map[i].type == JSON_Double)  // double
            {
                *((double*)(base_addr + map[i].offset)) = item->valuedouble;
            }
            else if (item->type == cJSON_Object && map[i].type == JSON_Struct)  // object
            {   
                int cnt = 0;
                i++;
                printf("item->next=%p\n", item->child);
                // calculate object item count;
                for (objitem = item->child; objitem != NULL; objitem = objitem->next, cnt++);
                printf("cnt=%d\n", cnt);

                for (j = 0; j < cnt && i < map_cnt; j++, i++)
                {
                    printf("[%d]key=%s, offset=%d, type=%d, item->type=%d\n", i, map[i].key,map[i].offset,map[i].type,item->type);
                    objitem =  JSON_GetObjectItem(item, map[i].key);
                    if (objitem->type == cJSON_String && map[i].type == JSON_String)       //string
                    {
                        if (objitem->valuestring)
                        {
                            strcpy((char*)(base_addr + map[i].offset), objitem->valuestring);
                        }
                    }
                    else if (objitem->type == cJSON_Number &&  map[i].type == JSON_Int)    // int 
                    {
                        *((int*)(base_addr + map[i].offset)) = objitem->valueint;
                    }
                    else if (objitem->type == cJSON_Number && map[i].type == JSON_Double)  // double
                    {
                        *((double*)(base_addr + map[i].offset)) = objitem->valuedouble;
                    }
                }
            }
        }
    }

    return 0;
}

