#ifndef _JSON_H__
#define _JSON_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "cJSON.h"
#include "DataType.h"

#define JSON_Int 1
#define JSON_String 2
#define JSON_Double 3
#define JSON_Struct 4 

//计算一个结构体内的成员的相对偏移地址
#define STRUCTOFFSET(structure, member) ((int)&((structure*)0)->member)

typedef cJSON JSON;

typedef struct
{
    S8 *key;
    S32 offset;
    S32 type;
}StructMap;

JSON* JSON_CreateObject(void);
JSON* JSON_CreateArray(void);
S32 JSON_GetArraySize(const JSON *array);
JSON* JSON_GetArrayItem(const JSON *array, int index);
JSON* JSON_GetObjectItem(const JSON * const object, const char * const string);
JSON* JSON_AddNumberToObject(JSON * const object, const char * const name, const double number);
JSON* JSON_AddStringToObject(JSON * const object, const char * const name, const char * const string);
JSON* JSON_Duplicate(const JSON *item, int recurse);
JSON* JSON_Parse(const char *value);
S8* JSON_Print(const JSON *item);
void JSON_AddItemToArray(JSON *array, JSON *item);
void JSON_AddItemToObject(JSON *object, const char *string, JSON *item);
void JSON_Delete(JSON *c);
S32 JSON_To_Struct(JSON *json, void *struct_addr, StructMap *map, int map_cnt);

#ifdef __cplusplus
}
#endif

#endif //_JSON_H__
