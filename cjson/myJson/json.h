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
#define STRUCTOFFSET(structure, member) ((S32)&((structure*)0)->member)

//对cJSON重命名
typedef cJSON JSON;

//定义映射表结构，用于描述把json结构映射到结构体的规则
typedef struct
{
    S8 *key;        //成员名称
    S32 offset;     //成员相对偏移量
    S32 type;       //成员类型，JSON_Int、JSON_String、JSON_Double、JSON_Struct
}StructMap;

//创建json结构
extern JSON* JSON_CreateObject(void);
extern JSON* JSON_CreateArray(void);
//获取json数组大小
extern S32 JSON_GetArraySize(const JSON *array);
//获取json item
extern JSON* JSON_GetArrayItem(const JSON *array, S32 index);
extern JSON* JSON_GetObjectItem(const JSON * const object, const S8 * const string);
//添加 item到json结构
extern JSON* JSON_AddNumberToObject(JSON * const object, const S8 * const name, const DOUBLE number);
extern JSON* JSON_AddStringToObject(JSON * const object, const S8 * const name, const S8 * const string);
extern void JSON_AddItemToArray(JSON *array, JSON *item);
extern void JSON_AddItemToObject(JSON *object, const S8 *string, JSON *item);
//拷贝json结构
extern JSON* JSON_Duplicate(const JSON *item, S32 recurse);
//string -> json
extern JSON* JSON_Parse(const S8 *value);
//json -> string
extern S8* JSON_Print(const JSON *item);
//释放json结构
extern void JSON_Delete(JSON *c);
//把一个json结构直接映射到结构体，目前最多支持二级json结构，且不支持映射json数组
extern S32 JSON_To_Struct(JSON *json, void *struct_addr, StructMap *map, S32 map_cnt);
/*  
假如有这样一个json结构
json =
{
    "name": "LiConghui",
    "age":  20,
    "weight":       58.6,
    "child":{
        "sex":  "girl",
        "height":  70,
    }
}
如果把上面的json映射到结构体，我们需要如下几步：
1、定义跟json结构相对应的结构体：
typedef struct{
    S8 sex[32];
    S32 height;
}Child;

typedef struct {
    S8 name[32];
    S32 age;
    DOUBLE weight;
    Child child;
}People;

2、定义映射表：
StructMap map[] = 
{
    {"name", STRUCTOFFSET(People, name), JSON_String},
    {"age", STRUCTOFFSET(People, age), JSON_Int},
    {"weight", STRUCTOFFSET(People, weight), JSON_Double},
    {"child", STRUCTOFFSET(People, child), JSON_Struct},
    {"sex", STRUCTOFFSET(People, child) + STRUCTOFFSET(Child, sex), JSON_String},
    {"height", STRUCTOFFSET(People, child) + STRUCTOFFSET(Child, height), JSON_Int},
}

3、接口调用：
{
    People people;
    JSON_To_Struct(json, &people, map, sizeof(map)/sizeof(map[0]));   
}
经过以上调用可得：
people.name = "LiConghui";
people.age = 20;
peopel.weight = 58.6;
people.child.sex = "girl";
people.child.height = 70;
*/

#ifdef __cplusplus
}
#endif

#endif //_JSON_H__
